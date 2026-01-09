cmake_minimum_required (VERSION 3.15)

# Set modern CMake policies
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15")
    cmake_policy(SET CMP0091 NEW)  # MSVC runtime library flags
    cmake_policy(SET CMP0092 NEW)  # MSVC warning flags
endif()

# Set vcpkg toolchain file if not already set
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
        # Try to find vcpkg installation
        find_program(VCPKG_EXECUTABLE vcpkg PATHS $ENV{VCPKG_ROOT}/vcpkg.exe)
        if(VCPKG_EXECUTABLE)
            get_filename_component(VCPKG_ROOT ${VCPKG_EXECUTABLE} DIRECTORY)
            set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
            message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        else()
            message(WARNING "vcpkg.json found but vcpkg executable not found. Please set CMAKE_TOOLCHAIN_FILE manually.")
        endif()
    endif()
endif()

# Verify vcpkg integration
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    message(STATUS "vcpkg integration active")
endif()

project (FicTrac)

# The version number.
set(FICTRAC_VERSION_MAJOR 2)
set(FICTRAC_VERSION_MIDDLE 1)
set(FICTRAC_VERSION_MINOR 2)
 
# output version info to be included by project
configure_file (
    "${PROJECT_SOURCE_DIR}/include/fictrac_version.h.in"
    "${PROJECT_SOURCE_DIR}/include/fictrac_version.h"
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

if (WIN32)
	message(STATUS "Building FicTrac for Windows")
elseif (UNIX)
	message(STATUS "Building FicTrac for Linux")
endif()

# dependency search dirs
set(OPENCV_DIR "." CACHE PATH "Path to OpenCV folder containing OpenCVConfig.cmake")
set(NLOPT_DIR "." CACHE PATH "Path to NLopt folder containing NLoptConfig.cmake")

# Add vcpkg installed packages to CMAKE_PREFIX_PATH
if(EXISTS "${PROJECT_SOURCE_DIR}/vcpkg_installed")
    if(WIN32)
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows")
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows/share")
        # Add vcpkg share directories to CMAKE_MODULE_PATH for Find*.cmake files
        file(GLOB VCPKG_SHARE_DIRS "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows/share/*")
        foreach(SHARE_DIR ${VCPKG_SHARE_DIRS})
            if(IS_DIRECTORY ${SHARE_DIR})
                list(APPEND CMAKE_MODULE_PATH ${SHARE_DIR})
            endif()
        endforeach()
    else()
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-linux")
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-linux/share")
        # Add vcpkg share directories to CMAKE_MODULE_PATH for Find*.cmake files
        file(GLOB VCPKG_SHARE_DIRS "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-linux/share/*")
        foreach(SHARE_DIR ${VCPKG_SHARE_DIRS})
            if(IS_DIRECTORY ${SHARE_DIR})
                list(APPEND CMAKE_MODULE_PATH ${SHARE_DIR})
            endif()
        endforeach()
    endif()
endif()

# optional build config
option(PGR_USB3 "Use Spinnaker SDK to capture from PGR USB3 cameras" OFF) # Disabled by default
option(PGR_USB2 "Use FlyCapture SDK to capture from PGR USB2 cameras" OFF) # Disabled by default
option(BASLER_USB3 "Use Pylon SDK to capture from Basler USB3 cameras" OFF) # Disabled by default

if(PGR_USB3)
    find_package(PGR REQUIRED)
elseif(PGR_USB2)
    set(PGR_DIR "." CACHE PATH "Path to PGR FlyCapture SDK folder")
elseif(BASLER_USB3)
    set(BASLER_DIR "." CACHE PATH "Path to Basler Pylon SDK folder")
endif()

# find dependencies
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${OPENCV_DIR} ${NLOPT_DIR})
find_package(OpenCV REQUIRED)
find_package(NLopt CONFIG REQUIRED)
if(NLopt_FOUND)
    get_filename_component(NLopt_INCLUDE_DIRS "${NLOPT_CONFIG_FILE}/../../../include" REALPATH)
    message(STATUS "Found NLopt: ${NLOPT_CONFIG_FILE}")
else()
    message(FATAL_ERROR "Error! Could not find NLopt lib at ${NLOPT_LIB}!")
endif()

if(WIN32)
    if(PGR_USB3)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PGR_DIR}/lib64/vs2015)
        find_library(PGR_LIB Spinnaker_v140.lib)
    elseif(PGR_USB2)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PGR_DIR}/lib64/vs2015)
        find_library(PGR_LIB FlyCapture2_v140.lib)
    elseif(BASLER_USB3)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${BASLER_DIR}/Development/lib/x64)
		find_library(BASLER_LIB PylonBase_MD_VC120_v5_0.lib)
		
		file(GLOB BASLER_LIBS ${BASLER_DIR}/Development/lib/x64/*.lib)
    endif()
    
else()  # gcc
    if(PGR_USB3)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PGR_DIR})
        find_library(PGR_LIB libSpinnaker.so)
    elseif(PGR_USB2)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PGR_DIR})
        find_library(PGR_LIB libflycapture.so)
    elseif(BASLER_USB3)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${BASLER_DIR}/lib64)
        find_library(BASLER_LIB libpylonbase.so)       
        
		file(GLOB BASLER_LIBS ${BASLER_DIR}/lib64/*.so)
    endif()
endif()

if(BASLER_USB3)
    get_filename_component(BASLER_DIR ${BASLER_LIB} DIRECTORY)
    get_filename_component(BASLER_DIR ${BASLER_DIR} DIRECTORY)    # step up 1 level
    if(WIN32)
        get_filename_component(BASLER_DIR ${BASLER_DIR} DIRECTORY)    # step up 1 level
    endif()
    if(BASLER_LIB)
        message(STATUS "Found Basler Pylon lib ${BASLER_LIB}")
    else()
        message(FATAL_ERROR "Error! Could not find Basler Pylon lib at ${BASLER_DIR}!")
    endif()
endif()

# add include dirs (will be moved to target-specific includes after target creation)
# include_directories(${PROJECT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS} ${NLopt_INCLUDE_DIRS})
set(FICTRAC_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS} ${NLopt_INCLUDE_DIRS})
if(PGR_USB2 OR PGR_USB3)
    if(WIN32)
        # include_directories(${PGR_DIR}/include)
        # include_directories(${PGR_INCLUDE_DIRS})
        list(APPEND FICTRAC_INCLUDE_DIRS ${PGR_INCLUDE_DIRS})
    else()
        if(PGR_USB2)
            # include_directories(${PGR_DIR}/include/flycapture)   # for ubuntu default install dir
            list(APPEND FICTRAC_INCLUDE_DIRS ${PGR_DIR}/include/flycapture)
        elseif(PGR_USB3)
            # include_directories(${PGR_DIR}/include/spinnaker)   # for ubuntu default install dir
            list(APPEND FICTRAC_INCLUDE_DIRS ${PGR_DIR}/include/spinnaker)
        endif()
    endif()
elseif(BASLER_USB3)
    # include_directories(${BASLER_DIR}/include)
    list(APPEND FICTRAC_INCLUDE_DIRS ${BASLER_DIR}/include)
endif()

# find sources to build
file(GLOB LIBFICTRAC_SRCS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp)

# add targets
add_library(fictrac_core STATIC ${LIBFICTRAC_SRCS})
add_executable(configGui ${PROJECT_SOURCE_DIR}/exec/configGui.cpp)
add_executable(fictrac ${PROJECT_SOURCE_DIR}/exec/fictrac.cpp)

# Set target-specific include directories
target_include_directories(fictrac_core PUBLIC ${FICTRAC_INCLUDE_DIRS})

# Set output directories as target properties
set_target_properties(fictrac_core PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

set_target_properties(configGui fictrac PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

# add preprocessor definitions
# PUBLIC means defs will be inherited by linked executables
target_compile_definitions(fictrac_core PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
if(WIN32)
    target_compile_definitions(fictrac_core PUBLIC _WIN32_WINNT=0x0A00)	# Win10
endif()
if(PGR_USB2)
    target_compile_definitions(fictrac_core PUBLIC PGR_USB2)
elseif(PGR_USB3)
    target_compile_definitions(fictrac_core PUBLIC PGR_USB3)
elseif(BASLER_USB3)
    target_compile_definitions(fictrac_core PUBLIC BASLER_USB3)
endif()

# add compile options
target_compile_features(fictrac_core PUBLIC cxx_std_17)

target_compile_options(fictrac_core PUBLIC 
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:
        /MP /GS /GL /W3 /wd4244 /WX- /Gy /Zc:wchar_t /O2 /Oi /Zc:inline /fp:precise /MD /EHsc
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -Ofast -w -fmessage-length=0 -Wno-unused-function -march=native
    >
)

if(WIN32)
    # Add corresponding linker options for Release builds
    target_link_options(fictrac_core PUBLIC $<$<CONFIG:Release>:/LTCG>)
endif()

# linking
target_link_libraries(fictrac_core PUBLIC 
    ${OpenCV_LIBS} 
    ${NLOPT_LIBRARIES}
    $<$<PLATFORM_ID:Windows>:Ws2_32>
    $<$<PLATFORM_ID:Linux>:pthread>
)

if(PGR_USB2 OR PGR_USB3)
    target_link_libraries(fictrac_core PUBLIC ${PGR_LIBRARIES})
elseif(BASLER_USB3)
    target_link_libraries(fictrac_core PUBLIC ${BASLER_LIBS})
endif()

# Link executables to core library (dependencies are handled automatically)
target_link_libraries(configGui PRIVATE fictrac_core)
target_link_libraries(fictrac PRIVATE fictrac_core)

# Apply linker options to executables on Windows
if(WIN32)
    target_link_options(configGui PUBLIC $<$<CONFIG:Release>:/LTCG>)
    target_link_options(fictrac PUBLIC $<$<CONFIG:Release>:/LTCG>)
endif()

# Copy required DLLs to output directory on Windows
if(WIN32)
    # Function to copy DLLs for a target
    function(copy_target_dlls TARGET_NAME)
        # Copy OpenCV DLLs
        if(OpenCV_FOUND)
            foreach(lib ${OpenCV_LIBS})
                get_target_property(lib_path ${lib} IMPORTED_LOCATION_RELEASE)
                if(lib_path)
                    get_filename_component(lib_dir ${lib_path} DIRECTORY)
                    file(GLOB opencv_dlls "${lib_dir}/../bin/*.dll")
                    foreach(dll ${opencv_dlls})
                        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${dll}" $<TARGET_FILE_DIR:${TARGET_NAME}>
                            COMMENT "Copying OpenCV DLL: ${dll}"
                        )
                    endforeach()
                endif()
            endforeach()
        endif()

        # Copy NLopt DLLs
        if(NLopt_FOUND)
            get_target_property(nlopt_lib NLopt::nlopt IMPORTED_LOCATION_RELEASE)
            if(nlopt_lib)
                get_filename_component(nlopt_dir ${nlopt_lib} DIRECTORY)
                file(GLOB nlopt_dlls "${nlopt_dir}/*.dll")
                foreach(dll ${nlopt_dlls})
                    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}" $<TARGET_FILE_DIR:${TARGET_NAME}>
                        COMMENT "Copying NLopt DLL: ${dll}"
                    )
                endforeach()
            endif()
        endif()

        # Copy PGR/Spinnaker DLLs
        if(PGR_USB3 AND PGR_LIBRARIES)
            foreach(lib ${PGR_LIBRARIES})
                get_filename_component(lib_path ${lib} REALPATH)
                get_filename_component(lib_dir ${lib_path} DIRECTORY)
                file(GLOB pgr_dlls "${lib_dir}/*.dll")
                foreach(dll ${pgr_dlls})
                    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}" $<TARGET_FILE_DIR:${TARGET_NAME}>
                        COMMENT "Copying PGR DLL: ${dll}"
                    )
                endforeach()
            endforeach()
        endif()

        # Copy Basler Pylon DLLs
        if(BASLER_USB3 AND BASLER_LIBS)
            foreach(lib ${BASLER_LIBS})
                get_filename_component(lib_path ${lib} REALPATH)
                get_filename_component(lib_dir ${lib_path} DIRECTORY)
                file(GLOB basler_dlls "${lib_dir}/*.dll")
                foreach(dll ${basler_dlls})
                    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}" $<TARGET_FILE_DIR:${TARGET_NAME}>
                        COMMENT "Copying Basler DLL: ${dll}"
                    )
                endforeach()
            endforeach()
        endif()

        # Copy vcpkg DLLs if using vcpkg
        if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
            if(EXISTS "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows/bin")
                file(GLOB vcpkg_dlls "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows/bin/*.dll")
                foreach(dll ${vcpkg_dlls})
                    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}" $<TARGET_FILE_DIR:${TARGET_NAME}>
                        COMMENT "Copying vcpkg DLL: ${dll}"
                    )
                endforeach()
            endif()
        endif()
    endfunction()

    # Apply DLL copying to executables
    copy_target_dlls(configGui)
    copy_target_dlls(fictrac)

    # Alternative: Use CMake's built-in DLL copying (CMake 3.21+)
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
        set_property(TARGET configGui PROPERTY 
            VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_PREFIX_PATH}/bin;$ENV{PATH}")
        set_property(TARGET fictrac PROPERTY 
            VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_PREFIX_PATH}/bin;$ENV{PATH}")
    endif()
endif()

# Optional: Create install rules for DLLs
if(WIN32)
    install(TARGETS configGui fictrac
        RUNTIME DESTINATION .
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    # Install required DLLs
    if(EXISTS "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows/bin")
        install(DIRECTORY "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows/bin/"
            DESTINATION .
            FILES_MATCHING PATTERN "*.dll"
        )
    endif()

    set(CPACK_PACKAGE_NAME "FicTrac")
    set(CPACK_PACKAGE_VENDOR "FicTrac")
    set(CPACK_PACKAGE_VERSION "2.2")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "webcam-based tracker of spherical motion")

    set(CPACK_GENERATOR "NSIS;ZIP")

    set(CPACK_NSIS_MODIFY_PATH ON)

    set(CPACK_NSIS_PACKAGE_NAME "FicTrac Tracking System")

    include(CPack)

endif()

