cmake_minimum_required(VERSION 3.16)
project(WaveRider VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# UTF-8 source encoding (suppress C4819 warnings on Chinese Windows)
add_compile_options(/utf-8)

# Qt5 — from conda (MSVC-compiled)
set(Qt5_DIR "C:/ProgramData/anaconda3/Library/lib/cmake/Qt5")
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg Network)

# BASS audio library
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(BASS REQUIRED)

add_subdirectory(src)
