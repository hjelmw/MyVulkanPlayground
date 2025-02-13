import SetupDependencies
import SetupProjects

from PrintUtils import Colors, PrintColor

print("Checking Dependencies...")
SetupDependencies.CheckAll()
print("Dependencies satisfied!")

print("Generating projects...")
SetupProjects.RunPremake()
PrintColor(Colors.OKGREEN, "Success! There should now be a .sln file in the root directory")