import SetupDependencies
import SetupProjects

from PrintUtils import Colors, PrintColor

print("Checking Dependencies...")
SetupDependencies.CheckAll()
print("Dependencies satisfied!")

print("Generating projects...")
SetupProjects.RunPremake()
PrintColor(Colors.OKGREEN, "There should now be a .sln file in the root directory")

if(SetupDependencies.vulkanIsInstalled is not True):
    PrintColor(Colors.FAIL, "However since a valid VulkanSDK install was not found the script has failed. Please visit https://vulkan.lunarg.com/ and install the latest one. You do not need to rerun the script but the solution will not work without it.")
