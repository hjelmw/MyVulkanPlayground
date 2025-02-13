import os
import zipfile
import urllib.request

from PrintUtils import Colors, PrintColor

def DownloadFile(packageURL, installPath):
    urllib.request.urlretrieve(packageURL, installPath)

def UnzipFile(newName, zipFilePath, unzipPath):
    with zipfile.ZipFile(zipFilePath, 'r') as zipRef:
        oldZipName = zipRef.infolist()[0].filename[:-1] # Skip last trailing "/" character
        print(oldZipName)
        zipRef.extractall(unzipPath)
        if (os.path.isdir("./vendor/" + oldZipName)):
            os.rename("./vendor/{}".format(oldZipName), "./vendor/{}".format(newName))

def DownloadGithubDependency(packageURL, packageName, packageVersion):
    githubInstallURL  = packageURL
    githubFolderName = "{}-{}".format(packageName, packageVersion)
    githubInstallPath = "./vendor/{}.zip".format(githubFolderName)
    print("Attempting download of {}...".format(packageURL))
    DownloadFile(packageURL, githubInstallPath)
    print("Extracting .zip into ./vendor/{}...".format(githubFolderName))
    UnzipFile(githubFolderName, githubInstallPath, "./vendor")
    print("Done! Deleting .zip...")
    os.remove(githubInstallPath)

def CheckVulkanInstalled(printNum, printTotal):
    vulkanSDKEnv = os.environ.get("VULKAN_SDK")
    if vulkanSDKEnv is None:
        PrintColor(Colors.WARNING, "({}, {})Did not find any active vulkan install. " 
                   + "Please visit https://vulkan.lunarg.com/ and install the latest Vulkan SDK. "
                   + "The installer will still work but you will likely not be able to compile the code if this is not fixed. ...".format(printNum, printTotal))
        return False
    else:
        PrintColor(Colors.OKBLUE, "({}, {}) Found active Vulkan install, skipped".format(printNum, printTotal))
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
