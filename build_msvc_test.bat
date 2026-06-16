@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d C:\Users\liwufan\Desktop\WaveRider
if exist build_msvc rmdir /s /q build_msvc
mkdir build_msvc
cd build_msvc
cmake .. -G "Visual Studio 17 2022" -A x64 -DQt5_DIR="C:/ProgramData/anaconda3/Library/lib/cmake/Qt5" 2>&1
