import sys
import subprocess
import colorama

from SetupPremake import Premake
from SetupVulkan import Vulkan
from SetupPython import Python
from colorama import Fore

def GenerateProjects():
    while(True):
        inputString = input("Please enter Visual Studio version [2019/2022]:")
        if(inputString == "2019" or inputString == "2022"):
            break 

    if (inputString == "2019"):
        subprocess.call("Win-GenProjects-vs2019.bat")
    elif (inputString == "2022"):
        subprocess.call("Win-GenProjects-vs2022.bat")


Python.CheckPython()
colorama.init()

Vulkan.CheckVulkan()
print("")
Premake.CheckPremake()

subprocess.call(["git", "submodule", "update", "--init", "--recursive"])
subprocess.call(["git", "lfs", "pull"])

sys.stdout.write(Fore.WHITE)
GenerateProjects()