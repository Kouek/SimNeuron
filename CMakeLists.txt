cmake_minimum_required(VERSION 3.16)

project("SimNeuron" LANGUAGES CXX C)

option("${PROJECT_NAME}_SIM_MOUSE" "Build mouse neuron simulation" ON)
option("${PROJECT_NAME}_SIM_WORM" "Build worm neuron simulation" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRDPARTY_DIR "${CMAKE_CURRENT_LIST_DIR}/3rd")
set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

# cfg injection
configure_file("${INCLUDE_DIR}/cmake_in" "${INCLUDE_DIR}/cmake_in.h")

# Qt dep
## Note 1:
## Append CMAKE_PREFIX_PATH with your Qt 5 installation prefix
## eg. In Windows, -DCMAKE_PREFIX_PATH="D:\Qt\Qt5.9.8\5.9.8\msvc2017_64\lib\cmake"
## Note 2:
## Make sure Qt5 shared libs path are in ENV_PATH
## eg. In Windows, append system/user PATH with "D:\Qt\Qt5.9.8\5.9.8\msvc2017_64\bin"
foreach(mod "Core" "Gui" "Widgets")
	find_package("Qt5" COMPONENTS ${mod} REQUIRED)
	list(APPEND Qt5_LIBS "Qt5::${mod}")
endforeach()
message(STATUS "Qt5_LIBS: ${Qt5_LIBS}")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# GLM dep
include("${THIRDPARTY_DIR}/glm.cmake")

# Util dep
include_directories(${INCLUDE_DIR})
include_directories(${THIRDPARTY_DIR})
include_directories("${THIRDPARTY_DIR}/glad/include")

add_subdirectory("src")
