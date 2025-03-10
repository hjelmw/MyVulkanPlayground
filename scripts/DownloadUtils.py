import os
import zipfile
import urllib.request
import shutil

from PrintUtils import Colors, PrintColor

def DownloadFile(packageURL, installPath):
    urllib.request.urlretrieve(packageURL, installPath)

def UnzipFile(packageName, packageNameWithVersion, zipFilePath, unzipPath):
    with zipfile.ZipFile(zipFilePath, 'r') as zipRef:
        oldZipName = zipRef.infolist()[0].filename[:-1] # Skip last trailing "/" character
        zipRef.extractall(unzipPath)

        # Rename folder to {packageName}-{version}, e.g glm-1.0.1
        if (os.path.isdir("./vendor/" + oldZipName)):
            os.rename("./vendor/{}".format(oldZipName), "./vendor/{}".format(packageNameWithVersion))


        fileIsExecutable = os.path.exists("./vendor/{}.exe".format(packageName))
        packageHasNestedFolder = os.path.isdir("./vendor/" + packageNameWithVersion + "/" + packageName)

        print("./vendor/{}.exe".format(packageNameWithVersion))
        print("./vendor" + packageNameWithVersion + "/" + packageName)

        print(fileIsExecutable)
        print(packageHasNestedFolder)

        # Add a root folder with only the package name and copy all files there if it does not exist
        # So that includes in vs can look like #include <glm/glm.hpp> instead of #include <glm-1.0.1/glm.hpp>
        if( not fileIsExecutable and not packageHasNestedFolder ):
            filesInCurrentDir = os.listdir("./vendor/" + packageNameWithVersion)
            os.mkdir("./vendor/" + packageNameWithVersion + "/" + packageName)      
            for fileInCurrentDir in filesInCurrentDir:
                filenameToMove = os.path.join("./vendor/" + packageNameWithVersion + "/", fileInCurrentDir)
                shutil.move(filenameToMove, "./vendor/" + packageNameWithVersion + "/" + packageName + "/" + fileInCurrentDir)


def DownloadGithubDependency(packageURL, packageName, packageVersion):
    githubInstallURL  = packageURL
    githubFolderName = "{}-{}".format(packageName, packageVersion)
    fileExtension = "zip" if packageURL[-3:] == "zip" else "exe"
    githubInstallPath = "./vendor/{}.{}".format(packageName, fileExtension)
    print("Attempting download of {}...".format(packageURL))
    DownloadFile(packageURL, githubInstallPath)
    if(fileExtension == "zip"):
        print("Extracting .zip into ./vendor/{}...".format(githubFolderName))
        UnzipFile(packageName, githubFolderName, githubInstallPath, "./vendor")
        print("Done! Deleting .zip...")
    os.remove(githubInstallPath)

def CheckVulkanInstalled(printNum, printTotal):
    vulkanSDKEnv = os.environ.get("VULKAN_SDK")
    if vulkanSDKEnv is None:
        PrintColor(Colors.WARNING, "Error Did not find any active vulkan install.")
        return False
    else:
        PrintColor(Colors.OKBLUE, "({}, {}) Found active Vulkan install.".format(printNum, printTotal))
        return True


def CheckAndDownloadGithubDependency(dependencyName, dependencyVersion, dependencyURL, printNum, printTotal):
    dependencyInstalled = os.path.isdir("./vendor/{}-{}".format(dependencyName, dependencyVersion))
    dependencyInstalled = dependencyInstalled or os.path.exists("./vendor/{}.exe".format(dependencyName, dependencyVersion))
    if not dependencyInstalled:
        PrintColor(Colors.WARNING, "({}, {}) Did not find {} \t".format(printNum, printTotal, dependencyName))
        DownloadGithubDependency(dependencyURL, dependencyName, dependencyVersion)
        PrintColor(Colors.OKGREEN, "{}-{} Succsesfully installed!".format(dependencyName, dependencyVersion))
    else:
        PrintColor(Colors.OKBLUE, "({}, {}) {} install found, skipped".format(printNum, printTotal, dependencyName))
