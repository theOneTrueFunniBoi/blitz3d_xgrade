## Blitz3D SoLoud: MAV-Less Edition

This is a fork of BlitzResearch' Blitz3D: SoLoud Edition which implements juanjp600's MAV-Less/Stacktrace/FastExt Killer/~~OpenAL~~ Patches

### Setting up SoLoud MAV-Less Project Files

You will need to install Microsoft Visual Studio, and the CMake and Git utilities. Any recent version of MSVC should work, although the latest version of Community Edition 2022 is reccomended.

You will also need to install the following optional MSVC components:
``` shell
"Desktop Development with C++",
"Windows Universal CRT SDK",
"C++ MFC for [VERSION] build tools (x86 & x64)",
"ASP.NET and web development prerequisites"
```

Then, from a shell/DOS prompt:

``` shell
git clone https://github.com/theOneTrueFunniBoi/mavless_soloud.git
cd mavless_soloud
cmake -S . -B cmake-build-release -A Win32 -G "Visual Studio 17 2022"
```

Assuming all went well, the project files should be built, and can be found in the "cmake-build-release" folder.

### Build Prerequisites

#### FMod

You will need to acquire the 3.75 32-bit version of "fmod.dll", then place it in "mavless_soloud\fmod375\lib".

For FMod licensing details, see [www.fmod.com/licensing](https://www.fmod.com/licensing).

### Building SoLoud MAV-Less

Simply open "Blitz3D.sln" and build the solution!

Or you could run the following shell/DOS command:
``` shell
cmake --build cmake-build-release --config Release
```

Assuming all went well, the "BLITZ3D_INSTALL" directory will contain the final binaries, simply run "SoLoud-MAVLess.exe" to get blitzing!

### To disable FMod
Open "CMakeLists.txt"
and change
``` shell
option(BB_FMOD_ENABLED "Blitz3D FMOD build enabled" ON)
```
to
``` shell
option(BB_FMOD_ENABLED "Blitz3D FMOD build enabled" OFF)
```

### Too lazy to build?

Well sorry, you're fresh outta luck! No pre-built builds available just yet...

### Want the original Blitz3D SoLoud?

You can grab standard SoLoud Edition prebuilt from [blitzresearch.itch.io/](https://blitzresearch.itch.io/)
