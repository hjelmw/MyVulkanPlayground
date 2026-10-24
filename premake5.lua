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
    
    dependson { "Shaders" } -- Technically this is not needed but it makes it so pressing F5 in VS also builds the shaders

    files { "./source/**.cpp", "./source/**.hpp" }

    files 
    { 
        "./vendor/imgui-docking/imgui/imgui*.h",
        "./vendor/imgui-docking/imgui/backends/imgui_impl_glfw.h",
        "./vendor/imgui-docking/imgui/backends/imgui_impl_vulkan.h",
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
        ["imgui/*"] = { "./vendor/imgui-docking/imgui/**.cpp", "./vendor/imgui-docking/imgui/**.h" }
    }

    defines 
    {
        "GLM_ENABLE_EXPERIMENTAL",
        "GLM_FORCE_RADIANS",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_XYZW_ONLY"
    }

    -- Returns complete name of dependency, i.e glm-3.2.1 etc  
    function find_dependency(pattern, exclude)
        for _, match in ipairs(os.matchdirs(pattern)) do
            if not exclude or not match:find(exclude, 1, true) then
                return match
            end
        end
        error("Missing dependency: " .. pattern)
    end

    -- Debug gets symbols, Release gets optimizations
    filter "configurations:Debug"
        symbols "On"
    filter "configurations:Release"
        optimize "On"
    filter {}

    includedirs
    {
        "$(SolutionDir)" .. find_dependency("vendor/glfw*") .. "/glfw/include",
        "$(SolutionDir)" .. find_dependency("vendor/glm-*", "glm-aabb"),
        "$(SolutionDir)" .. find_dependency("vendor/glm-aabb*"),
        "$(SolutionDir)" .. find_dependency("vendor/imgui*") .. "/imgui", -- This is needed because the backend does relative includes :(
        "$(SolutionDir)" .. find_dependency("vendor/stbi*"),
        "$(SolutionDir)" .. find_dependency("vendor/tinyobjloader*"),
        "$(VULKAN_SDK)/Include",
        "./source/VulkanEngine",
        "./shaders", -- For the shared shader constants file
    }

    links 
    {
        "$(VULKAN_SDK)/Lib/vulkan-1.lib",
        "$(SolutionDir)" .. find_dependency("vendor/glfw*") .. "/glfw/lib-vc2022/glfw3.lib",
    }


project "Shaders"
    kind "Utility" -- Only custom build steps
    objdir "build/%{cfg.buildcfg}/obj"

    files { "./shaders/*.vert", "./shaders/*.frag", "./shaders/*.comp", "./shaders/**.hpp" }

    local shaderCommand = "\"$(VULKAN_SDK)/Bin/glslc.exe\" --target-env=vulkan1.3 %{cfg.buildcfg == 'Release' and '-O' or '-g'} -o \"$(SolutionDir)%(Identity).spv\" \"%(Identity)\""

    filter "files:shaders/**.vert or shaders/**.frag or shaders/**.comp"
        buildmessage "Compiling %(Identity)"
        buildinputs { "$(SolutionDir)shaders/ShaderConstants.hpp" } -- Modifying the shared shader constants rebuilds all shaders
        buildcommands { shaderCommand }
        buildoutputs { "$(SolutionDir)%(Identity).spv" }

    filter {}
