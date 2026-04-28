cmake_minimum_required(VERSION 3.21)

project(FicTrac
    VERSION 2.1.2
    DESCRIPTION "Webcam-based fictive path integration for tethered insects"
    LANGUAGES CXX)

# Prefer BoostConfig.cmake over the deprecated FindBoost module on CMake 3.30+
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Project-local Find modules (FindSpinnaker.cmake, ...)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32)
    message(STATUS "Building FicTrac for Windows")
elseif(UNIX)
    message(STATUS "Building FicTrac for Linux")
endif()

# version header (written into the build tree, not the source tree)
configure_file(
    "${PROJECT_SOURCE_DIR}/include/fictrac_version.h.in"
    "${PROJECT_BINARY_DIR}/generated/fictrac_version.h"
    @ONLY)

# optional camera-SDK build switches
option(SPINNAKER   "Use Spinnaker SDK to capture from FLIR/PGR cameras" OFF)
option(FLYCAPTURE  "Use FlyCapture SDK to capture from legacy PGR USB2 cameras" OFF)
option(PYLON       "Use Pylon SDK to capture from Basler cameras"               OFF)

# find dependencies
# Rely on CMAKE_PREFIX_PATH (set by pixi via $CONDA_PREFIX or by vcpkg's
# toolchain file) to locate config-mode packages. No custom *_DIR cache
# variables — pass `-DCMAKE_PREFIX_PATH=...` explicitly if needed.
find_package(OpenCV REQUIRED)
find_package(NLopt CONFIG REQUIRED)
# Boost.Asio is used header-only in src/SocketRecorder.cpp and
# src/SerialRecorder.cpp; no compiled Boost components are needed.
find_package(Boost REQUIRED)

# camera SDKs as INTERFACE IMPORTED targets
if(SPINNAKER OR FLYCAPTURE OR PYLON)
    add_library(fictrac_camera_sdk INTERFACE)
endif()

if(SPINNAKER)
    # Auto-detects /opt/spinnaker, /usr, etc. Override with -DSpinnaker_ROOT=
    # (or the legacy -DPGR_DIR=) when the SDK is in a non-standard location.
    find_package(Spinnaker REQUIRED)
    target_link_libraries(fictrac_camera_sdk INTERFACE Spinnaker::Spinnaker)
    target_compile_definitions(fictrac_camera_sdk INTERFACE SPINNAKER)
elseif(FLYCAPTURE)
    set(PGR_DIR "" CACHE PATH "Path to PGR FlyCapture SDK folder")
    if(WIN32)
        find_library(PGR_LIB
            NAMES FlyCapture2_v140
            PATHS "${PGR_DIR}/lib64/vs2015"
            REQUIRED)
        target_include_directories(fictrac_camera_sdk INTERFACE "${PGR_DIR}/include")
    else()
        find_library(PGR_LIB
            NAMES flycapture
            PATHS "${PGR_DIR}/lib"
            REQUIRED)
        target_include_directories(fictrac_camera_sdk INTERFACE "${PGR_DIR}/include/flycapture")
    endif()
    target_link_libraries(fictrac_camera_sdk INTERFACE "${PGR_LIB}")
    target_compile_definitions(fictrac_camera_sdk INTERFACE FLYCAPTURE)
    message(STATUS "Found PGR FlyCapture lib: ${PGR_LIB}")
elseif(PYLON)
    set(BASLER_DIR "" CACHE PATH "Path to Basler Pylon SDK folder")
    # Auto-detects common pylon installs. Override with -DPylon_ROOT=
    # (or the legacy -DBASLER_DIR=) when the SDK is in a non-standard location.
    find_package(Pylon REQUIRED)
    target_link_libraries(fictrac_camera_sdk INTERFACE Pylon::Pylon)
    target_compile_definitions(fictrac_camera_sdk INTERFACE PYLON)
    if(Pylon_VERSION)
        message(STATUS "Found Basler Pylon ${Pylon_VERSION}")
    else()
        message(STATUS "Found Basler Pylon")
    endif()
endif()

# find sources to build
file(GLOB LIBFICTRAC_SRCS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")

# targets
add_library(fictrac_core STATIC ${LIBFICTRAC_SRCS})
add_executable(configGui "${PROJECT_SOURCE_DIR}/exec/configGui.cpp")
add_executable(fictrac   "${PROJECT_SOURCE_DIR}/exec/fictrac.cpp")

# include dirs (target-scoped)
target_include_directories(fictrac_core PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated>")
# OpenCV's CMake config does not always populate imported-target include dirs,
# so add OpenCV_INCLUDE_DIRS explicitly.
target_include_directories(fictrac_core SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})

# preprocessor definitions
# PUBLIC so executables inherit them
target_compile_definitions(fictrac_core PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
if(WIN32)
    target_compile_definitions(fictrac_core PUBLIC _WIN32_WINNT=0x0A00) # Win10
endif()

# compile options
if(WIN32)
    target_compile_options(fictrac_core PUBLIC
        $<$<CONFIG:Release>:/MP /GS /GL /w /WX- /Gy /Zc:wchar_t /O2 /Oi /Zc:inline /fp:precise /MD /EHsc /std:c++17>)
else()
    target_compile_options(fictrac_core PUBLIC
        -Ofast -w -c -fmessage-length=0 -std=c++17 -Wno-unused-function -march=native -MMD)
endif()

# linking
target_link_libraries(fictrac_core PUBLIC
    ${OpenCV_LIBS}
    NLopt::nlopt
    Boost::headers)
if(WIN32)
    target_link_libraries(fictrac_core PUBLIC Ws2_32)
else()
    target_link_libraries(fictrac_core PUBLIC pthread)
endif()
if(TARGET fictrac_camera_sdk)
    target_link_libraries(fictrac_core PRIVATE fictrac_camera_sdk)
endif()

target_link_libraries(configGui PRIVATE fictrac_core)
add_dependencies(configGui fictrac_core)
target_link_libraries(fictrac PRIVATE fictrac_core)
add_dependencies(fictrac fictrac_core)

# if(WIN32)
    # set_target_properties(configGui PROPERTIES LINK_FLAGS /LTCG)
    # set_target_properties(fictrac PROPERTIES LINK_FLAGS /LTCG)
# endif()
