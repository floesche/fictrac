#############################################################################
#
# This software is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
#
# Description:
# Try to find Basler Pylon library API
#
# PYLON_FOUND
# PYLON_INCLUDE_DIRS
# PYLON_LIBRARIES
# PYLON_VERSION
#
# Authors:
# Wenfeng CAI
# Fabien Spindler : compat with OSX and Windows
#
# Original source code copied from ViSP:
# https://github.com/lagadic/visp/blob/17245ff1f4f0874415d0db9f075955471f2cd459/cmake/FindPylon.cmake
# See http://visp.inria.fr for more information about the original code
#
#############################################################################

if(APPLE)
  find_path(PYLON_INCLUDE_DIR pylon/PylonIncludes.h)
  find_path(PYLON_BASE_INCLUDE_DIR
    NAMES Base/GCTypes.h
    HINTS ${PYLON_INCLUDE_DIR}/Headers
    PATH_SUFFIXES GenICam)

  find_library(PYLON_LIBRARIES pylon)

  find_program(PYLON_CONFIG pylon-config
               HINTS ${PYLON_INCLUDE_DIR}
               PATH_SUFFIXES Versions/A/Resources/Tools)

  if(PYLON_INCLUDE_DIR)
    list(APPEND PYLON_INCLUDE_DIRS ${PYLON_INCLUDE_DIR})
  endif()
  if(PYLON_BASE_INCLUDE_DIR)
    list(APPEND PYLON_INCLUDE_DIRS ${PYLON_BASE_INCLUDE_DIR})
  endif()

  if(PYLON_CONFIG)
    execute_process(COMMAND ${PYLON_CONFIG} "--version"
                    OUTPUT_VARIABLE PYLON_VERSION_TMP
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE "-n" "" PYLON_VERSION ${PYLON_VERSION_TMP})
  endif()

  if(PYLON_INCLUDE_DIRS AND PYLON_LIBRARIES)
    set(PYLON_FOUND TRUE)
  endif()

  mark_as_advanced(PYLON_BASE_INCLUDE_DIR)
elseif(MSVC)
  find_path(PYLON_INCLUDE_DIR pylon/PylonIncludes.h
    PATHS "$ENV{PYLON_HOME}/include"
        "C:/Program Files/Basler/pylon 5/Development/include"
        "C:/Program Files/Basler/pylon 6/Development/include")

  set(PYLON_LIB_SEARCH_PATH "$ENV{PYLON_HOME}/lib/x64")

  if(CMAKE_CL_64)
    list(APPEND PYLON_LIB_SEARCH_PATH "C:/Program Files/Basler/pylon 5/Development/lib/x64")
    list(APPEND PYLON_LIB_SEARCH_PATH "C:/Program Files/Basler/pylon 6/Development/lib/x64")
  else()
    list(APPEND PYLON_LIB_SEARCH_PATH "C:/Program Files/Basler/pylon 5/Development/lib/Win32")
    list(APPEND PYLON_LIB_SEARCH_PATH "C:/Program Files/Basler/pylon 6/Development/lib/Win32")
  endif()

  find_library(PYLON_BASE_LIBRARY
    NAMES PylonBase_v6_3.lib PylonBase_v6_1.lib PylonBase_v5_1.lib PylonBase_MD_VC120_v5_0.lib
    PATHS ${PYLON_LIB_SEARCH_PATH})
  find_library(PYLON_GCBASE_LIBRARY
    NAMES GCBase_MD_VC141_v3_1_Basler_pylon.lib GCBase_MD_VC141_v3_1_Basler_pylon_v5_1.lib GCBase_MD_VC120_v3_0_Basler_pylon_v5_0.lib
    PATHS ${PYLON_LIB_SEARCH_PATH})
  find_library(PYLON_GENAPI_LIBRARY
    NAMES GenApi_MD_VC141_v3_1_Basler_pylon.lib GenApi_MD_VC141_v3_1_Basler_pylon_v5_1.lib GenApi_MD_VC120_v3_0_Basler_pylon_v5_0.lib
    PATHS ${PYLON_LIB_SEARCH_PATH})
  find_library(PYLON_UTILITY_LIBRARY
    NAMES PylonUtility_v6_3.lib PylonUtility_v6_1.lib PylonUtility_v5_1.lib PylonUtility_MD_VC120_v5_0.lib
    PATHS ${PYLON_LIB_SEARCH_PATH})

  if(PYLON_INCLUDE_DIR)
    list(APPEND PYLON_INCLUDE_DIRS ${PYLON_INCLUDE_DIR})
  endif()
  if(PYLON_BASE_LIBRARY AND PYLON_GCBASE_LIBRARY)
    list(APPEND PYLON_LIBRARIES ${PYLON_BASE_LIBRARY})
    list(APPEND PYLON_LIBRARIES ${PYLON_GCBASE_LIBRARY})
    list(APPEND PYLON_LIBRARIES ${PYLON_GENAPI_LIBRARY})
    list(APPEND PYLON_LIBRARIES ${PYLON_UTILITY_LIBRARY})
  endif()

  if(PYLON_INCLUDE_DIRS AND PYLON_LIBRARIES)
    vp_parse_header("${PYLON_INCLUDE_DIR}/pylon/PylonVersionNumber.h" PYLON_VERSION_LINES PYLON_VERSION_MAJOR PYLON_VERSION_MINOR PYLON_VERSION_SUBMINOR)
    set(PYLON_VERSION "${PYLON_VERSION_MAJOR}.${PYLON_VERSION_MINOR}.${PYLON_VERSION_SUBMINOR}")
    set(PYLON_FOUND TRUE)
    message(STATUS "Using Basler Pylon version " ${PYLON_VERSION})
  endif()

  mark_as_advanced(
    PYLON_BASE_LIBRARY
    PYLON_GCBASE_LIBRARY
    PYLON_GENAPI_LIBRARY
    PYLON_UTILITY_LIBRARY
  )

elseif(UNIX)
  set(PYLON_ROOT_SEARCH_PATH
    /opt/pylon5
    /opt/pylon
  )

  # For more possible versions, just add more paths below.
  # list(APPEND PYLON_ROOT_SEARCH_PATH "/somepath/include")

  find_program(PYLON_CONFIG pylon-config
               PATHS ${PYLON_ROOT}
               PATHS $ENV{PYLON_ROOT}
               PATHS ${PYLON_ROOT_SEARCH_PATH}
               PATH_SUFFIXES bin)

  if(PYLON_CONFIG)
    execute_process(COMMAND ${PYLON_CONFIG} "--version"
                    OUTPUT_VARIABLE PYLON_VERSION
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PYLON_CONFIG} "--libs" "--libs-rpath"
                    OUTPUT_VARIABLE PYLON_LIBRARIES
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PYLON_CONFIG} "--cflags-only-I"
                    OUTPUT_VARIABLE PYLON_INC_TMP
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE "-I" "" PYLON_INCLUDE_DIRS ${PYLON_INC_TMP})

    set(PYLON_FOUND TRUE)
  endif()
endif()

mark_as_advanced(
  PYLON_INCLUDE_DIR
  PYLON_INCLUDE_DIRS
  PYLON_LIBRARIES
  PYLON_CONFIG
)
