# MyVulkanPlayground
This repository contains the code and assets for my hobby graphics engine where I experiment with implementing modern rendering techniques and systems. It's written in C++, uses Vulkan for the graphics API and GLSL for shaders. The code is developed based on my own collecetd understanding through personal experience and not simply sourced from a single course 

# Features
In this project I have implemented several systems meant for simplifying interaction with the [Graphics Pipeline](https://en.wikipedia.org/wiki/Graphics_pipeline) and exposed them an interface I call "Draw Nodes". 

There are helpers for handling keyboard/mouse inputs, resource state tracking, pipeline management & shader binding tables as well as a rudimentary model rendering system. The base class is overridable and has two functions: Init and Draw. Init is called once on startup while Draw runs once per frame. The entire viewport is additionally rendered through ImGui and you can inspect all textures in real time.

You can have a look under [source/VulkanEngine/DrawNodes/](https://github.com/hjelmw/MyVulkanPlayground/tree/main/source/VulkanEngine/DrawNodes
) to see what rendering techniques have currently been implemented.

![alt text](engine.png)

# Setup
1. Install [Python](https://www.python.org/downloads/) (Version 3.7 or above should work)
2. Install [VulkanSDK](https://vulkan.lunarg.com/) from Lunarg (Version 1.4 should work)
4. `CreateVisualStudioSolution.bat`. It should download all the other required dependencies and create the .sln file
5. Open in Visual Studio 2022 (have not tested older versions but it might still work)


**Note:** When installing the Vulkan SDK you can optionally choose to install the GLM math library but regardless the build script will download and use its own version

## Dependencies
* Vulkan SDK (Your graphics driver needs to support `VK_KHR_DYNAMIC_RENDERING`)
* Python3
* Premake5
* GLFW
* GLM
* ImGUI
* TinyOBJLoader
* stbi_image

