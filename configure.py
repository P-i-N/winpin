# This script configures the Premake script on the first run, if it hasn't been configured yet.
# It will try to determine solution ("workspace") name and project structure from the directory structure.
#
# By default, the solution will be named after the directory running this script.
#
# The structure of individual projects inside the solution will be determined by the structure of
# the "src" subdirectory. If there is a "src" subdirectory, it will be assumed that the solution
# contains multiple projects, each of which is a subdirectory of "src". Otherwise, it will be
# assumed that the solution contains a single project, which is the directory running this script.
#
# The script will also try to determine the type of each project. If there is a "main.cpp" or "main.c"
# (case insensitive) file in the project directory, it will be assumed that the project is an executable.
# Otherwise, it will be assumed that the project is a library.
#
# Projects having the same name as the solution will be assumed to be the "main" project. If this
# main project is a library, it will be linked to all other projects in the solution as well.
#
# The script will also look into ".gitignore" file and modify it to ignore ".bin" and ".build"
# folders generated by Premake. If the exceptions already exist, they will not be added again.
#
# If "premake5.exe" is not found, the script will download it from GitHub, extract it into
# ".build" folder and run it from there.
#
# As mentioned above, this is only done on the first run. If you want to reconfigure the solution,
# delete the generated "premake5.lua" file and rerun this script. After the "premake5.lua" file is
# generated, you can do modify it manually. It will not be overwritten by this script.
#
# Ooof... that's a lot of text. Let's get to the code running!

import os
import re
import shutil


# Replace backslashes with forward slashes in the path
def forward_slashes(path):
    return path.replace("\\", "/")


# Check, if ".bin" and ".build" folders exist and create them, if they don't
def check_folders():
    # Create ".bin" folder, if it doesn't exist
    if not os.path.isdir(".bin"):
        os.mkdir(".bin")

    # Create ".build" folder, if it doesn't exist
    if not os.path.isdir(".build"):
        os.mkdir(".build")


# Modify ".gitignore" file to ignore ".bin" and ".build" folders
def modify_gitignore():
    gitignore_path = ".gitignore"
    if not os.path.isfile(gitignore_path):
        return

    with open(gitignore_path, "r") as gitignore_file:
        gitignore = gitignore_file.read()

    append = ""

    if re.search("^\.bin$", gitignore, re.MULTILINE) is None:
        append += ".bin\n"

    if re.search("^\.build$", gitignore, re.MULTILINE) is None:
        append += ".build\n"

    if append != "":
        print("Modifying .gitignore file: added .bin and .build folders")

        append = "\n# Premake generated folders\n" + append

        with open(gitignore_path, "w") as gitignore_file:
            gitignore_file.write(gitignore + append)


# Check, if "premake5.exe" is accessible and download it from GitHub into ".build" folder, if it isn't
def check_or_install_premake():
    path = shutil.which("premake5.exe")
    if not path is None:
        return path

    if not os.path.isfile(".build/premake5.exe"):
        print("Premake not found. Downloading it from GitHub...")
        import urllib.request
        import zipfile
        import io

        premake_url = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip"

        # Download the latest Premake release from GitHub
        with urllib.request.urlopen(premake_url) as response:
            with zipfile.ZipFile(io.BytesIO(response.read())) as zip_file:
                # Extract "premake5.exe" from archive into ".build" folder
                zip_file.extract("premake5.exe", ".build")

        print("Premake downloaded successfully!")

    return ".build/premake5.exe"


# Project class
class Project:
    def __init__(self, name, path, type):
        self.name = name
        self.path = forward_slashes(path)
        self.type = type

    def __str__(self):
        return "Project: " + self.name + ", " + self.path + ", " + self.type


# Generate Lua project definition
def generate_project_def(solution_name, dir):
    project_name = os.path.basename(dir)
    content = 'project "{0}"\n'.format(project_name)
    content += '    language "C++"\n'

    # If project contains "main.c" or "main.cpp" file, it is an "ConsoleApp", otherwise it is a "StaticLib"
    if os.path.isfile(os.path.join(dir, "main.c")) or os.path.isfile(os.path.join(dir, "main.cpp")):
        content += '    kind "ConsoleApp"\n'
    else:
        content += '    kind "StaticLib"\n'

    content += '    files {{ "{0}/**.*" }}\n'.format(dir)
    content += '    removefiles {{ "{0}/**.aps" }}\n'.format(dir)
    content += '    includedirs {{ "{0}" }}\n'.format(dir)

    if solution_name != project_name:
        content += '    links {{ "{0}" }}\n'.format(solution_name)

    return content + "\n"

# Generate "premake5.lua" file
def generate_premake_file():
    # Determine solution name from the directory name
    solution_name = os.path.basename(os.getcwd())
    print("Solution name: " + solution_name)

    content = '''workspace "{0}"
    -- Premake output folder
    location(path.join(".build", _ACTION))

    -- Target architecture
    architecture "x86_64"

    -- Configuration settings
    configurations {{ "Debug", "Release" }}

    -- Debug configuration
    filter {{ "configurations:Debug" }}
        defines {{ "DEBUG" }}
        symbols "On"
        optimize "Off"

    -- Release configuration
    filter {{ "configurations:Release" }}
        defines {{ "NDEBUG" }}
        optimize "Speed"
        inlining "Auto"

    filter {{ "language:not C#" }}
        defines {{ "_CRT_SECURE_NO_WARNINGS" }}
        characterset ("MBCS")
        cppdialect "C++latest"
        staticruntime "on"

    filter {{ }}
        targetdir ".bin/%{{cfg.longname}}/"
        defines {{ "WIN32", "_AMD64_" }}
        exceptionhandling "Off"
        rtti "Off"
        vectorextensions "AVX2"

-------------------------------------------------------------------------------

includedirs {{ "libs", "include", "src" }}

'''.format(solution_name)

    # If "src" subdirectory does not exist, take the current directory as the only executable project
    if not os.path.isdir("src"):
        content += generate_project_def(solution_name, ".")
    else:
        for project_name in os.listdir("src"):
            project_path = os.path.join("src", project_name)
            content += generate_project_def(solution_name, forward_slashes(project_path))

    # Write content of "premake5.lua" file
    with open("premake5.lua", "w") as premake_file:
        premake_file.write(content)

    return


# Entry point
def main():
    check_folders()
    modify_gitignore()

    premake_path = os.path.normpath(check_or_install_premake())

    print("Using Premake from: " + premake_path)

    if not os.path.isfile("premake5.lua"):
        generate_premake_file()
    
    # If "premake5.lua" exists, just run Premake from 'premake_path' with "vs2022" argument and exit
    os.system(premake_path + " vs2022")
    return

if __name__ == "__main__":
    main()
