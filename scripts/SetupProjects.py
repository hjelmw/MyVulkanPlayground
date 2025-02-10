import subprocess
from PrintUtils import Colors, PrintColor

def RunPremake():
    returnCode = subprocess.call("./vendor/premake5.exe vs2022")
    if returnCode != 0:
        PrintColor(Colors.FAIL, "Premake error: Something went wrong when trying to generate the VS solution")