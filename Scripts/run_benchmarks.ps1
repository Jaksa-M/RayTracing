# Runs the Editor benchmark for every combination of scene, BVH method, device (CPU/GPU) and camera view, Repeats times.
# Every run is a separate Editor process, so runs don't affect each other. All results are appended to one CSV file.
# Build Editor in Release first. Examples (from the repository folder):
#   powershell -ExecutionPolicy Bypass -File Scripts\run_benchmarks.ps1
#   powershell -ExecutionPolicy Bypass -File Scripts\run_benchmarks.ps1 -Repeats 10 -Devices gpu -Scenes crytek_sponza
#   powershell -ExecutionPolicy Bypass -File Scripts\run_benchmarks.ps1 -DryRun
param(
    [int]$Repeats = 5,
    [int]$Frames = 100,
    [int]$Warmup = 10,
    [int]$Bounces = 3,
    [int]$Width = 1024,
    [int]$Height = 768,
    [string[]]$Methods = @("midpoint", "sah", "binned_sah"),
    [string[]]$Devices = @("cpu", "gpu"),
    [string[]]$Scenes = @(),   # names from the $scene_list below, empty means all
    [int]$MaxAttempts = 3,     # a failed run (crash, error) is started again, at most this many times in total
    [int]$TimeoutMinutes = 30, # a run that takes longer is stopped and counts as failed
    [string]$Out = "",         # CSV file, default is Benchmarks\results.csv
    [switch]$DryRun            # only print the runs, don't start them
)

# Scenes to benchmark. Obj is relative to the Editor folder. Views are camera preset names saved in the Editor,
# empty Views means the initial camera of the scene. Example: Views = @("crytek_1", "crytek_2", "crytek_3")
$scene_list = @(
    @{ Name = "dabrovic_sponza"; Obj = "../Resources/sponza/sponza.obj";        Scale = 1.0;  Views = @() },
    @{ Name = "crytek_sponza";   Obj = "../Resources/crytek_sponza/sponza.obj"; Scale = 0.01; Views = @() }
)

$ErrorActionPreference = "Stop"
$invariant = [Globalization.CultureInfo]::InvariantCulture # numbers always with "." (Serbian Windows would write 0,01)

# "-Devices cpu,gpu" given through -File arrives as one string "cpu,gpu", so it is split here
$Methods = @($Methods -split ",")
$Devices = @($Devices -split ",")
$Scenes = @($Scenes -split "," | Where-Object { $_ -ne "" })

$root = Split-Path -Parent $PSScriptRoot
$editor_dir = Join-Path $root "Editor"
$exe = Join-Path $root "x64\Release\Editor.exe" # Visual Studio builds into the solution folder
$presets_file = Join-Path $root "Cameras\saved_presets.txt"
if ($Out -eq "") { $Out = Join-Path $root "Benchmarks\results.csv" }
elseif (-not [IO.Path]::IsPathRooted($Out)) { $Out = Join-Path (Get-Location) $Out }
$log_dir = Join-Path $root ("Benchmarks\logs\" + (Get-Date -Format "yyyy-MM-dd_HH-mm-ss"))

# ---------- Checks before starting, so a long run doesn't fail in the middle ----------
$problems = @()
if (-not (Test-Path $exe)) { $problems += "Editor.exe not found: $exe (build Editor in Release)" }
foreach ($m in $Methods) { if ($m -notin @("midpoint", "sah", "binned_sah")) { $problems += "Unknown method '$m' (midpoint, sah, binned_sah)" } }
foreach ($d in $Devices) { if ($d -notin @("cpu", "gpu")) { $problems += "Unknown device '$d' (cpu, gpu)" } }

$selected_scenes = @($scene_list | Where-Object { $Scenes.Count -eq 0 -or $_.Name -in $Scenes })
foreach ($s in $Scenes) { if ($s -notin $scene_list.Name) { $problems += "Unknown scene '$s' (" + ($scene_list.Name -join ", ") + ")" } }

$preset_names = @()
if (Test-Path $presets_file) {
    $preset_names = @(Get-Content $presets_file | Where-Object { $_ -match "^Name " } | ForEach-Object { $_.Substring(5) })
}
foreach ($scene in $selected_scenes) {
    if (-not (Test-Path (Join-Path $editor_dir $scene.Obj))) { $problems += "OBJ file not found: $($scene.Obj)" }
    foreach ($view in $scene.Views) {
        if ($view -notin $preset_names) { $problems += "Camera preset '$view' of $($scene.Name) not found in $presets_file" }
    }
}
if ($problems.Count -gt 0) {
    $problems | ForEach-Object { Write-Host "ERROR: $_" -ForegroundColor Red }
    exit 1
}

# Warn if code was changed after Editor.exe was built, the results would be from old code
$exe_time = (Get-Item $exe).LastWriteTime
$newer = @(Get-ChildItem (Join-Path $root "Renderer"), $editor_dir -Recurse -Include *.cpp, *.h | Where-Object { $_.LastWriteTime -gt $exe_time })
if ($newer.Count -gt 0) {
    Write-Host "WARNING: $($newer.Count) source file(s) changed after Editor.exe was built (e.g. $($newer[0].Name)). Rebuild Release?" -ForegroundColor Yellow
}

# ---------- List of runs ----------
# Repeat is the outer loop, so every combination is measured at different times (PC getting warm affects all of them equally)
$runs = @()
for ($r = 1; $r -le $Repeats; $r++) {
    foreach ($scene in $selected_scenes) {
        $views = @($scene.Views)
        if ($views.Count -eq 0) { $views = @("") }
        foreach ($method in $Methods) {
            foreach ($device in $Devices) {
                foreach ($view in $views) {
                    $runs += [pscustomobject]@{ Repeat = $r; Scene = $scene; Method = $method; Device = $device; View = $view }
                }
            }
        }
    }
}

function Get-RunLabel($run) {
    $view_name = if ($run.View -eq "") { "initial" } else { $run.View }
    return "{0} {1} {2} {3} repeat {4}" -f $run.Scene.Name, $run.Method, $run.Device, $view_name, $run.Repeat
}

Write-Host "Benchmark: $($runs.Count) runs, $Frames frames (+$Warmup warmup), ${Width}x${Height}, $Bounces bounces"
Write-Host "Results: $Out"
if ($DryRun) {
    for ($i = 0; $i -lt $runs.Count; $i++) { Write-Host ("[{0}/{1}] {2}" -f ($i + 1), $runs.Count, (Get-RunLabel $runs[$i])) }
    exit 0
}
Write-Host "Logs: $log_dir"
New-Item -ItemType Directory -Force $log_dir | Out-Null

function Get-LineCount($path) {
    if (Test-Path $path) { return @(Get-Content $path).Count }
    return 0
}

# ---------- Running ----------
$failed = @()
$total_time = [Diagnostics.Stopwatch]::StartNew()
for ($i = 0; $i -lt $runs.Count; $i++) {
    $run = $runs[$i]
    $label = Get-RunLabel $run

    $arguments = "--benchmark --obj `"$($run.Scene.Obj)`" --scale $($run.Scene.Scale.ToString($invariant))" +
                 " --method $($run.Method) --device $($run.Device) --width $Width --height $Height --bounces $Bounces" +
                 " --warmup $Warmup --frames $Frames --out `"$Out`""
    if ($run.View -ne "") { $arguments += " --view `"$($run.View)`"" }

    $ok = $false
    for ($attempt = 1; $attempt -le $MaxAttempts -and -not $ok; $attempt++) {
        Write-Host ("[{0}/{1}] {2} (attempt {3}) ... " -f ($i + 1), $runs.Count, $label, $attempt) -NoNewline
        $log = Join-Path $log_dir ("{0:D4}_{1}_attempt{2}" -f ($i + 1), ($label -replace " ", "_"), $attempt)
        $lines_before = Get-LineCount $Out
        $run_time = [Diagnostics.Stopwatch]::StartNew()

        $p = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $editor_dir -PassThru `
                           -RedirectStandardOutput "$log.out.txt" -RedirectStandardError "$log.err.txt"
        $null = $p.Handle # without this PowerShell loses the exit code
        if ($p.WaitForExit($TimeoutMinutes * 60 * 1000)) { $result = "exit code $($p.ExitCode)" }
        else {
            Stop-Process -Id $p.Id -Force
            $result = "timeout after $TimeoutMinutes min"
        }

        # Success means the Editor wrote its row (even if it crashed afterwards while closing)
        if ((Get-LineCount $Out) -gt $lines_before) {
            $ok = $true
            Write-Host ("ok ({0:N0} s)" -f $run_time.Elapsed.TotalSeconds) -ForegroundColor Green
        } else {
            Write-Host "FAILED ($result), log: $log.err.txt" -ForegroundColor Red
        }
    }
    if (-not $ok) { $failed += $label }
}

Write-Host ("Done in {0:N1} min: {1} of {2} runs ok" -f $total_time.Elapsed.TotalMinutes, ($runs.Count - $failed.Count), $runs.Count)
if ($failed.Count -gt 0) {
    Write-Host "Failed runs (after $MaxAttempts attempts):" -ForegroundColor Red
    $failed | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}
exit 0
