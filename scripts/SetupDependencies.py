import os
import DownloadUtils

from PrintUtils import Colors, PrintColor

vulkanSDKVersion     = "1.4.304.0"
premakeVersion       = "5.0.0-beta4"
glfwVersion          = "3.4"
glmVersion           = "1.0.1"
glmaabbVersion       = "master"
imguiVersion         = "docking" #"1.91.8"
tinyobjloaderVersion = "2.0"
stbiVersion          = "master" # Repo does not do releases
vsWhereVersion       = "3.1.7"


premakeGithubURL       = "https://github.com/premake/premake-core/releases/download/v{}/premake-{}-windows.zip".format(premakeVersion, premakeVersion)
glfwGithubURL          = "https://github.com/glfw/glfw/releases/download/{}/glfw-{}.bin.WIN64.zip".format(glfwVersion, glfwVersion)
glmGithubURL           = "https://github.com/g-truc/glm/releases/download/{}/glm-{}-light.zip".format(glmVersion, glmVersion)
glmaabbGithubURL       = "https://github.com/hjelmw/glm-aabb/archive/refs/heads/master.zip" # Repo does not do releases
imguiGithubURL         = "https://github.com/ocornut/imgui/archive/refs/heads/docking.zip"
tinyobjloaderGithubURL = "https://github.com/tinyobjloader/tinyobjloader/archive/refs/tags/v{}-rc1.zip".format(tinyobjloaderVersion)
stbiGithubURL          = "https://github.com/nothings/stb/archive/refs/heads/master.zip" # Repro does not do releases
vsWhereGithubURL       = "https://github.com/microsoft/vswhere/releases/download/{}/vswhere.exe".format(vsWhereVersion)

def CheckVulkanInstalled(printNum, printTotal):
    vulkanSDKEnv = os.environ.get("VULKAN_SDK")
    if vulkanSDKEnv is None:
        PrintColor(Colors.FAIL, "Error Did not find any active vulkan install.")
    else:
        PrintColor(Colors.OKBLUE, "({}, {}) Found active Vulkan install.".format(printNum, printTotal))

def CheckAll():
    vendorDirectoryExists = os.path.exists("./vendor/")
    if (not vendorDirectoryExists):
        os.mkdir("./vendor")
    
    itemsChecked = 0
    totalItemsToCheck = 7

    # 1. Check Vulkan SDK
    CheckVulkanInstalled(itemsChecked, totalItemsToCheck)
    itemsChecked += 1

    # 2. Check premake
    DownloadUtils.CheckAndDownloadGithubDependency("premake5", premakeVersion, premakeGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1

    # 3. Check GLFW
    DownloadUtils.CheckAndDownloadGithubDependency("glfw", glfwVersion, glfwGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1

    # 4. Check GLM
    DownloadUtils.CheckAndDownloadGithubDependency("glm", glmVersion, glmGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1

    DownloadUtils.CheckAndDownloadGithubDependency("glm-aabb", glmaabbVersion, glmaabbGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1

    # 5. Check ImGui
    DownloadUtils.CheckAndDownloadGithubDependency("imgui", imguiVersion, imguiGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1

    # 6. Check tinyobjloader
    DownloadUtils.CheckAndDownloadGithubDependency("tinyobjloader", tinyobjloaderVersion, tinyobjloaderGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1
    
    # 7. Check stbi_image
    DownloadUtils.CheckAndDownloadGithubDependency("stbi", stbiVersion, stbiGithubURL, itemsChecked, totalItemsToCheck)
    itemsChecked +=1

    #8. Check VSWhere
    #DownloadUtils.CheckAndDownloadGithubDependency("vswhere", vsWhereVersion, vsWhereGithubURL, itemsChecked, totalItemsToCheck)
    #itemsChecked +=1
