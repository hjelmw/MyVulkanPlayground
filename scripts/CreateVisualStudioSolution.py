import SetupDependencies
import SetupProjects

from PrintUtils import Colors, PrintColor

print("Checking Dependencies...")
SetupDependencies.CheckAll()
print("Dependencies satisfied!")

print("Generating projects...")
SetupProjects.RunPremake()
PrintColor(Colors.OKGREEN, "Success! There should now be a .sln file in the root directory")

if(not SetupDependencies.vulkanIsInstalled):
    PrintColor(Colors.WARNING,
               "However since Vulkan SDK was not found the code will likely not compile and run! " 
               + "Please ensure you have installed it or alternatively ignore this if you believe this is incorrect "
               + "I may have written bad vulkan check code as well")