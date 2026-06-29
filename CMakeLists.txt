cmake_minimum_required(VERSION 3.21)

project(FicTrac
    VERSION 2.1.2
    DESCRIPTION "Webcam-based fictive path integration for tethered insects"
    LANGUAGES CXX)

# Prefer BoostConfig.cmake over the deprecated FindBoost module on CMake 3.30+
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
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
        set(_flycapture_hints
            ${PGR_DIR}
            $ENV{PGR_DIR}
            "C:/Program Files/Point Grey Research/FlyCapture2"
            "C:/Program Files/FLIR Systems/FlyCapture2")
        find_library(PGR_LIB
            NAMES FlyCapture2_v143 FlyCapture2_v142 FlyCapture2_v141 FlyCapture2_v140 FlyCapture2
            HINTS ${_flycapture_hints}
            PATH_SUFFIXES lib64/vs2022 lib64/vs2019 lib64/vs2017 lib64/vs2015 lib64 lib
            REQUIRED)
        find_path(PGR_INCLUDE_DIR
            NAMES FlyCapture2.h
            HINTS ${_flycapture_hints}
            PATH_SUFFIXES include
            REQUIRED)
        target_include_directories(fictrac_camera_sdk INTERFACE "${PGR_INCLUDE_DIR}")
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
target_compile_features(fictrac_core PUBLIC cxx_std_17)

# compile options
if(MSVC)
    # Suppress vcpkg global integration: pixi manages all dependencies and
    # vcpkg's auto-injected headers/libs would conflict with pixi's packages.
    set(CMAKE_VS_GLOBALS "UseVcpkg=false")
    target_compile_options(fictrac_core PUBLIC
        /EHsc /GS /WX- /Zc:inline /Zc:wchar_t /fp:precise /w
        $<$<CONFIG:Release>:/Gy /O2 /Oi>)
else()
    target_compile_options(fictrac_core PUBLIC
        -Ofast -w -c -fmessage-length=0 -Wno-unused-function -march=native -MMD)
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

# On Windows, generate a .bat launcher next to each executable.
# The launcher prepends the runtime DLL directories to PATH for that
# invocation so the exe finds its DLLs without requiring the pixi
# environment to be active or DLLs to be copied into the build tree.
#
# Order matters. Camera-SDK bin dirs come FIRST: the FLIR/Teledyne SDKs
# ship private copies of libcurl.dll, libiomp5md.dll and ucrtbase.dll that
# their own DLLs require. If pixi's Library/bin (which ships different
# versions of those same DLLs) is searched first, the SDK DLL loads the
# wrong copy and fails with STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139).
# pixi's Library/bin goes last; its OpenCV/ffmpeg/etc. DLLs have unique
# names so nothing shadows them.
# Paths are resolved at configure time when pixi is active.
if(WIN32)
    set(_runtime_dll_dirs "")
    # Camera-SDK runtime dir is derived from the linked import lib by
    # swapping .../lib64/... for .../bin64/... (the SDKs mirror this layout).
    if(SPINNAKER AND Spinnaker_LIBRARY)
        get_filename_component(_sdk_lib_dir "${Spinnaker_LIBRARY}" DIRECTORY)
        string(REPLACE "/lib64/" "/bin64/" _sdk_bin_dir "${_sdk_lib_dir}")
        list(APPEND _runtime_dll_dirs "${_sdk_bin_dir}")
    elseif(FLYCAPTURE AND PGR_LIB)
        get_filename_component(_sdk_lib_dir "${PGR_LIB}" DIRECTORY)
        string(REPLACE "/lib64/" "/bin64/" _sdk_bin_dir "${_sdk_lib_dir}")
        list(APPEND _runtime_dll_dirs "${_sdk_bin_dir}")
    endif()
    list(APPEND _runtime_dll_dirs "$ENV{CONDA_PREFIX}/Library/bin")

    # Build the semicolon-separated PATH prefix with native (backslash) paths.
    set(_launcher_path_prefix "")
    foreach(_dir IN LISTS _runtime_dll_dirs)
        file(TO_NATIVE_PATH "${_dir}" _native_dir)
        string(APPEND _launcher_path_prefix "${_native_dir};")
    endforeach()

    foreach(_target fictrac configGui)
        file(GENERATE
            OUTPUT "$<TARGET_FILE_DIR:${_target}>/${_target}.bat"
            CONTENT "@echo off\nset \"PATH=${_launcher_path_prefix}%PATH%\"\n\"%~dp0${_target}.exe\" %*\n")
    endforeach()
endif()
