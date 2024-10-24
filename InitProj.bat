@echo off
cmake -S . -B cmake-build-release -A Win32 -G "Visual Studio 17 2022"
echo Done! Check above for errors.
echo Press any key to exit.
pause > NUL