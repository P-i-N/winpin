workspace "winpin"
    -- Premake output folder
    location(path.join(".build", _ACTION))

    -- Target architecture
    architecture "x86_64"

    -- Configuration settings
    configurations { "Debug", "Release" }

    -- Debug configuration
    filter { "configurations:Debug" }
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    -- Release configuration
    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "Size"
        inlining "Auto"

    filter { "language:not C#" }
        defines { "_CRT_SECURE_NO_WARNINGS" }
        characterset ("MBCS")
        cppdialect "C++latest"
        staticruntime "on"

    filter { }
        targetdir ".bin/%{cfg.longname}/"
        defines { "WIN32", "_AMD64_" }
        exceptionhandling "Off"
        rtti "Off"
        vectorextensions "AVX2"

-------------------------------------------------------------------------------

includedirs { "libs", "include", "src" }

project "winpin"
    targetname "WinPin"
    language "C++"
    kind "WindowedApp"
    files { "src/winpin/**.*" }
    removefiles { "src/winpin/**.aps" }
    includedirs { "src/winpin" }
