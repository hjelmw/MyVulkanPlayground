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
    
    postbuildcommands 
    {
         "{COPYFILE} %[build/%{cfg.buildcfg}/VulkanEngine.exe] %[./VulkanEngine_%{cfg.buildcfg}.exe]"
    }
    
    files { "./source/**.cpp", "./source/**.hpp" }
    files { "./shaders/*.vert", "shaders/*.frag" }

    files 
    { 
        "./vendor/imgui-docking/imgui*.h",
        "./vendor/imgui-docking/backends/imgui_impl_glfw.hpp",
        "./vendor/imgui-docking/backends/imgui_impl_vulkan.hpp",
        "./vendor/imgui-docking/imgui*.cpp",
        "./vendor/imgui-docking/backends/imgui_impl_glfw.cpp",
        "./vendor/imgui-docking/backends/imgui_impl_vulkan.cpp"
    }

    -- Setup the filters. Splits the files into header and source but keeps original folder structure
    vpaths
    {
        ["headers/*"] = { "./source/**.hpp" },
        ["source/*"] = { "./source/**.cpp" },
        ["imgui/*"] = {"./vendor/imgui-docking/.**cpp" }
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
        "./source/VulkanEngine",
        "$(VULKAN_SDK)/Include",
        "$(SolutionDir)" .. find_dependency("vendor/glfw*") .. "/include",
        "$(SolutionDir)" .. find_dependency("vendor/glm*"),
        "$(SolutionDir)" .. find_dependency("vendor/imgui*"),
        "$(SolutionDir)" .. find_dependency("vendor/stb*"),
        "$(SolutionDir)" .. find_dependency("vendor/tinyobjloader*")
    }

    links 
    {
        "$(VULKAN_SDK)/Lib/vulkan-1.lib",
        "$(SolutionDir)" .. find_dependency("vendor/glfw*") .. "/lib-vc2022/glfw3.lib",
    }

    filter "files:shaders/**.vert"
        buildmessage "Compiling vertex shader"
        buildcommands "$(VULKAN_SDK)\\Bin\\glslangValidator -g -V -o $(SolutionDir)\\%(Identity).spv %(Identity)"
        buildoutputs "$(SolutionDir)\\%(Identity).spv"

    filter "files:shaders/**.frag"
        buildmessage "Compiling fragment shader"
        buildcommands "$(VULKAN_SDK)\\Bin\\glslangValidator -g -V -o $(SolutionDir)\\%(Identity).spv %(Identity)"
        buildoutputs "$(SolutionDir)\\%(Identity).spv"

