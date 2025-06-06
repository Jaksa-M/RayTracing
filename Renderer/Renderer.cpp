// Renderer.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "Renderer.h"


// This is an example of an exported variable
RENDERER_API int nRenderer=0;

// This is an example of an exported function.
RENDERER_API int fnRenderer(void)
{
    return 1;
}

// This is the constructor of a class that has been exported.
CRenderer::CRenderer()
{
    return;
}
