workspace "MyVulkanPlayground"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "VulkanEngine"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
debugdir = "./"

project "VulkanEngine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir "build/%{cfg.buildcfg}"
    objdir "build/%{cfg.buildcfg}/obj"
    flags "MultiProcessorCompile"

    buildoptions 
    {
        "/Zc:__cplusplus" -- Changes __cplusplus macro so glm stops complaining about C++ version (MSVC defines it as 199711L regardless of actual version)
    }

    postbuildcommands 
    {
         "{COPYFILE} %[build/%{cfg.buildcfg}/VulkanEngine.exe] %[./VulkanEngine_%{cfg.buildcfg}.exe]"
    }
    
    files { "./source/**.cpp", "./source/**.hpp" }
    files { "./shaders/*.vert", "shaders/*.frag" }

    files 
    { 
        "./vendor/imgui-docking/imgui/imgui*.h",
        "./vendor/imgui-docking/imgui/backends/imgui_impl_glfw.hpp",
        "./vendor/imgui-docking/imgui/backends/imgui_impl_vulkan.hpp",
        "./vendor/imgui-docking/imgui/imgui*.cpp",
        "./vendor/imgui-docking/imgui/backends/imgui_impl_glfw.cpp",
        "./vendor/imgui-docking/imgui/backends/imgui_impl_vulkan.cpp",
        "./vendor/glm-aabb-master/glm-aabb/AABB.hpp",
        "./vendor/glm-aabb-master/glm-aabb/AABB.cpp"
    }

    -- Setup the filters. Splits the files into header and source but keeps original folder structure
    vpaths
    {
        ["headers/*"] = { "./source/**.hpp" },
        ["source/*"] = { "./source/**.cpp" },
        ["imgui/*"] = {"./vendor/imgui-docking/imgui/.**cpp" }
    }

    defines 
    {
        "GLM_ENABLE_EXPERIMENTAL",
        "GLM_FORCE_RADIANS",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_XYZW_ONLY"
    }

    -- Returns complete name of dependency, i.e glm-3.2.1 etc  
    function find_dependency(pattern)
        local matches = os.matchdirs(pattern)
        if #matches > 0 then
            return matches[1]
        else
            return nil
        end
    end



    includedirs
    {
        "$(SolutionDir)" .. find_dependency("vendor/glfw*") .. "/glfw/include",
        "$(SolutionDir)" .. find_dependency("vendor/glm*"),
        "$(SolutionDir)" .. find_dependency("vendor/glm-aabb*"),
        "$(SolutionDir)" .. find_dependency("vendor/imgui*") .. "/imgui", -- This is needed because the backend does relative includes :(
        "$(SolutionDir)" .. find_dependency("vendor/stbi*"),
        "$(SolutionDir)" .. find_dependency("vendor/tinyobjloader*"),
        "$(VULKAN_SDK)/Include",
        "./source/VulkanEngine",
    }

    links 
    {
        "$(VULKAN_SDK)/Lib/vulkan-1.lib",
        "$(SolutionDir)" .. find_dependency("vendor/glfw*") .. "/glfw/lib-vc2022/glfw3.lib",
    }

    filter "files:shaders/**.vert"
        buildmessage "Compiling vertex shader"
        buildcommands "$(VULKAN_SDK)\\Bin\\glslangValidator -g -V -o $(SolutionDir)\\%(Identity).spv %(Identity)"
        buildoutputs "$(SolutionDir)\\%(Identity).spv"

    filter "files:shaders/**.frag"
        buildmessage "Compiling fragment shader"
        buildcommands "$(VULKAN_SDK)\\Bin\\glslangValidator -g -V -o $(SolutionDir)\\%(Identity).spv %(Identity)"
        buildoutputs "$(SolutionDir)\\%(Identity).spv"

