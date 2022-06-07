import os
import sys
import subprocess
import urllib.request as urlReq
import pkg_resources

vulkanURL = "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe"

def Install(package):
    subprocess.check_call(['python', '-m', 'pip', 'install', package])

def ValidatePackage(package):
    required = { package }
    installed = { pkg.key for pkg in pkg_resources.working_set }
    missing = required - installed

    if (missing):
        Install(package)

ValidatePackage('colorama')

import colorama
from colorama import Fore, Back, Style

colorama.init()

def IsProcessOpen(processName):
    progs = str(subprocess.check_output('tasklist'))
    if processName in progs:
        return True
    else:
        return False

def ProgressBar(a, b, c):
    progressBarLength = 34
    progress = (progressBarLength * a * b / c)
    
    sys.stdout.write('\r')
    sys.stdout.write('[')
    
    for i in range(int(progress)):
        sys.stdout.write('█')

    for i in range(progressBarLength - int(progress)):
        sys.stdout.write('-')
    sys.stdout.write(']')
    sys.stdout.write(f"{round(100*a*b/c, 1)}%" + " / 100%")

def InstallVulkan():
    print(Fore.GREEN + "Downloading Vulkan SDK installer...")

    opener = urlReq.build_opener()
    opener.addheaders = [('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.102 Safari/537.36')]
    urlReq.install_opener(opener)

    vulkanExeName = "VulkanSDK.exe"
    result = urlReq.urlretrieve(vulkanURL, vulkanExeName, ProgressBar)
    print("\n")

    print("Download finished! Running installer!")

    while(True):
        subprocess.call(vulkanExeName, shell=True)

        while(True):
            if (IsProcessOpen(vulkanExeName) == False):
                break
        
        inputStr = input(Fore.WHITE + "Was the Vulkan SDK installed properly? [" + Fore.GREEN + "Y" + Fore.WHITE + "/" + Fore.RED + "N" + Fore.WHITE + "]:")
        if (inputStr.lower() == "y"):
            break

    os.remove(vulkanExeName)

def GenerateProjects():
    while(True):
        inputString = input("Please enter Visual Studio version [2019/2022]:")
        if(inputString == "2019" or inputString == "2022"):
            break 

    if (inputString == "2019"):
        subprocess.call("Win-GenProjects-vs2019.bat")
    elif (inputString == "2022"):
        subprocess.call("Win-GenProjects-vs2022.bat")

vulkanPath = str(os.getenv("VULKAN_SDK"))

if (vulkanPath == "None"):
    print(Fore.RED + "Vulkan SDK not installed! Installing!")
    InstallVulkan()

elif(vulkanPath.find("1.3.") == -1):
    print(Fore.RED + "Correct Vulkan version not found! Installing 1.3.xxx")
    InstallVulkan()

else:
    print(Fore.GREEN + "Correct Vulkan version found!")


sys.stdout.write(Fore.WHITE)
GenerateProjects()