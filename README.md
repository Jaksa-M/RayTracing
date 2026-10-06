# Ray Tracer

A ray tracer in C++20 with a small editor around it. It renders on the CPU across several threads, or on the GPU with OpenGL compute shaders, and you can switch between the two while it is running.

## Features

- path tracing with a configurable number of bounces and samples per pixel; frames are accumulated, so the image gets cleaner while the camera stands still
- Lambertian, metal and emissive materials, textures, and an HDR environment map for lighting
- triangle meshes loaded from OBJ files (Sponza, Cornell box and a teapot are included)
- BVH acceleration with my own builder, using either midpoint split or the surface area heuristic. Each mesh has its own tree and there is one more tree over all meshes. tinybvh is also wired in so the two can be compared.
- multithreaded CPU rendering in blocks, with a selectable block size
- a GPU renderer written as a compute shader

## Editor

The editor is built with Dear ImGui. From it you can:

- switch scenes, the BVH method, and CPU or GPU rendering
- change the bounce count, how many pixels are traced per frame, and the environment light
- view debug output instead of the final image: geometric or shading normals, depth, UVs, roughness
- draw debug rays and freeze the camera to look at them from another angle
- save camera presets and jump between them
- take a screenshot, also as HDR or at higher quality
- reload shaders without restarting

The camera moves with `W` `A` `S` `D`, `Q` and `E`.

## Layout

```
Renderer/           static library: scenes, BVH, materials, CPU and GPU rendering
Editor/             the application: window, UI and camera control
Renderer_Tests/     Google Test project that checks scene loading
ShaderFiles/        compute, vertex and fragment shaders
Resources/          models, textures and the environment map
ExternalLibraries/  GLFW, glad, Dear ImGui, stb_image, tinyobjloader, tinybvh
```

## Build

Windows, Visual Studio 2022.

1. Open `RayTracingTutorial.sln`.
2. Select the x64 configuration and set `Editor` as the startup project.
3. Build and run.

All libraries are in the repo, so nothing has to be installed. Google Test for `Renderer_Tests` is restored through NuGet. The GPU path needs a graphics card with OpenGL 4.3 or newer.

Model and texture credits are in the `copyright.txt` files under `Resources/`.

## Note
For now, works only with AMD GPU's.
