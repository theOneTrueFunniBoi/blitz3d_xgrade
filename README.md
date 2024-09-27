## Blitz3D SoLoud - MAVLess Edition

This is a fork of BlitzResearch' Blitz3D SoLoud Edition which implements juanjp600's MAVLess Patch

### Building SoLoud MAVLess from source

You will need to install Microsoft Visual Studio, and the CMake and Git utilities. Any recent version of MSVC should be OK, I am currently using Community Edition 2022.

You will also need to install the following optional MSVC components: "Desktop development with C++", "MFC and ATL support" and "ASP.NET and web development".

Then, from a DOS prompt:

``` shell
git clone https://github.com/theOneTrueFunniBoi/mavless_soloud.git
cd mavless_soloud
cmake -S . -B cmake-build-release -A Win32 -G "Visual Studio 17 2022"
cmake --build cmake-build-release --config Release
```
Assuming all went well, the BLITZ3D_INSTALL directory will contain the final binaries, simply run Blitz3D.exe to get blitzing!

### Too lazy to build?

Well sorry, you're fresh outta luck! No pre-built builds available just yet...

### Want the original Blitz3D SoLoud?

You can grab standard SoLoud Edition prebuilt from https://blitzresearch.itch.io/
