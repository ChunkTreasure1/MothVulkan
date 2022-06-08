from re import sub
import sys
import subprocess

from SetupPython import Python

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

import colorama

from colorama import Fore
from SetupPremake import Premake
from SetupVulkan import Vulkan

colorama.init()

subprocess.call(["git", "lfs", "pull"])
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

Vulkan.CheckVulkan()
print("")
Premake.CheckPremake()

subprocess.call(["git", "submodule", "update", "--init", "--recursive"])
subprocess.call(["git", "lfs", "pull"])

sys.stdout.write(Fore.WHITE)
GenerateProjects()