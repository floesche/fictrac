# FindPylon.cmake
#
# Locates the Basler pylon SDK. Basler does not ship a portable CMake package
# for all supported platforms, so this module searches the standard install
# locations and consumes pylon-config on Unix-like systems.
#
# Hints (any of these are honoured if set):
#   - Pylon_ROOT  (cache var or env, recommended)
#   - PYLON_ROOT  (cache var or env, alias)
#   - BASLER_DIR  (cache var or env, legacy alias)
#   - PYLON_HOME  (env, legacy alias)
#
# Result variables:
#   Pylon_FOUND         - true if SDK was found
#   Pylon_INCLUDE_DIRS  - header search paths
#   Pylon_LIBRARIES     - libraries to link
#   Pylon_LIBRARY_DIRS  - library search paths from pylon-config (Unix)
#   Pylon_LINK_OPTIONS  - extra linker flags from pylon-config (Unix)
#   Pylon_VERSION       - SDK version, if discoverable
#
# Imported target:
#   Pylon::Pylon

set(_pylon_hints
    ${Pylon_ROOT}
    ${PYLON_ROOT}
    ${BASLER_DIR}
    $ENV{Pylon_ROOT}
    $ENV{PYLON_ROOT}
    $ENV{BASLER_DIR}
    $ENV{PYLON_HOME})
list(REMOVE_ITEM _pylon_hints "")
list(REMOVE_DUPLICATES _pylon_hints)

function(_pylon_pick_newest out_var)
    set(_candidates ${ARGN})
    if(_candidates)
        list(SORT _candidates COMPARE NATURAL ORDER DESCENDING)
        list(GET _candidates 0 _picked)
        set(${out_var} "${_picked}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

function(_pylon_extract_version header out_var)
    if(NOT EXISTS "${header}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    file(STRINGS "${header}" _version_lines
        REGEX "#define PYLON_VERSION_(MAJOR|MINOR|SUBMINOR|BUILD)[ \t]+[0-9]+")
    foreach(_line IN LISTS _version_lines)
        if(_line MATCHES "#define PYLON_VERSION_MAJOR[ \t]+([0-9]+)")
            set(_v_major "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "#define PYLON_VERSION_MINOR[ \t]+([0-9]+)")
            set(_v_minor "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "#define PYLON_VERSION_SUBMINOR[ \t]+([0-9]+)")
            set(_v_patch "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "#define PYLON_VERSION_BUILD[ \t]+([0-9]+)")
            set(_v_build "${CMAKE_MATCH_1}")
        endif()
    endforeach()

    if(DEFINED _v_major AND DEFINED _v_minor AND DEFINED _v_patch)
        set(_version "${_v_major}.${_v_minor}.${_v_patch}")
        if(DEFINED _v_build)
            string(APPEND _version ".${_v_build}")
        endif()
        set(${out_var} "${_version}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

if(WIN32)
    file(GLOB _pylon_default_paths LIST_DIRECTORIES true
        "C:/Program Files/Basler/pylon*"
        "C:/Program Files (x86)/Basler/pylon*")
    list(SORT _pylon_default_paths COMPARE NATURAL ORDER DESCENDING)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_pylon_arch_dir x64)
    else()
        set(_pylon_arch_dir Win32)
    endif()

    find_path(Pylon_INCLUDE_DIR
        NAMES pylon/PylonIncludes.h
        HINTS ${_pylon_hints}
        PATHS ${_pylon_default_paths}
        PATH_SUFFIXES Development/include include)

    set(_pylon_lib_dirs)
    foreach(_root IN LISTS _pylon_hints _pylon_default_paths)
        list(APPEND _pylon_lib_dirs
            "${_root}/Development/lib/${_pylon_arch_dir}"
            "${_root}/lib/${_pylon_arch_dir}")
    endforeach()

    if(Pylon_INCLUDE_DIR)
        get_filename_component(_pylon_include_parent "${Pylon_INCLUDE_DIR}" DIRECTORY)
        get_filename_component(_pylon_root_from_include "${_pylon_include_parent}" DIRECTORY)
        list(APPEND _pylon_lib_dirs
            "${_pylon_include_parent}/lib/${_pylon_arch_dir}"
            "${_pylon_root_from_include}/Development/lib/${_pylon_arch_dir}")
    endif()

    list(REMOVE_DUPLICATES _pylon_lib_dirs)
    foreach(_dir IN LISTS _pylon_lib_dirs)
        if(NOT IS_DIRECTORY "${_dir}")
            continue()
        endif()

        file(GLOB _pylon_base_candidates    "${_dir}/PylonBase*.lib")
        file(GLOB _pylon_gcbase_candidates  "${_dir}/GCBase*.lib")
        file(GLOB _pylon_genapi_candidates  "${_dir}/GenApi*.lib")
        file(GLOB _pylon_utility_candidates "${_dir}/PylonUtility*.lib")

        _pylon_pick_newest(Pylon_BASE_LIBRARY    ${_pylon_base_candidates})
        _pylon_pick_newest(Pylon_GCBASE_LIBRARY  ${_pylon_gcbase_candidates})
        _pylon_pick_newest(Pylon_GENAPI_LIBRARY  ${_pylon_genapi_candidates})
        _pylon_pick_newest(Pylon_UTILITY_LIBRARY ${_pylon_utility_candidates})

        if(Pylon_BASE_LIBRARY AND Pylon_GCBASE_LIBRARY)
            file(GLOB Pylon_LIBRARIES "${_dir}/*.lib")
            set(Pylon_LIBRARY_DIRS "${_dir}")
            break()
        endif()
    endforeach()

    if(Pylon_INCLUDE_DIR)
        set(Pylon_INCLUDE_DIRS "${Pylon_INCLUDE_DIR}")
        _pylon_extract_version("${Pylon_INCLUDE_DIR}/pylon/PylonVersionNumber.h"
            Pylon_VERSION)
    endif()

    set(Pylon_LIBRARY "${Pylon_BASE_LIBRARY}")
else()
    file(GLOB _pylon_default_paths LIST_DIRECTORIES true "/opt/pylon*")
    list(APPEND _pylon_default_paths /usr/local /usr)
    list(SORT _pylon_default_paths COMPARE NATURAL ORDER DESCENDING)

    find_program(Pylon_CONFIG
        NAMES pylon-config
        HINTS ${_pylon_hints}
        PATHS ${_pylon_default_paths}
        PATH_SUFFIXES bin)

    if(Pylon_CONFIG)
        execute_process(
            COMMAND "${Pylon_CONFIG}" --version
            OUTPUT_VARIABLE Pylon_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND "${Pylon_CONFIG}" --cflags-only-I
            OUTPUT_VARIABLE _pylon_include_flags
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND "${Pylon_CONFIG}" --libs --libs-rpath
            OUTPUT_VARIABLE _pylon_link_flags
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        separate_arguments(_pylon_include_args UNIX_COMMAND "${_pylon_include_flags}")
        foreach(_arg IN LISTS _pylon_include_args)
            if(_arg MATCHES "^-I(.+)")
                list(APPEND Pylon_INCLUDE_DIRS "${CMAKE_MATCH_1}")
            endif()
        endforeach()

        separate_arguments(_pylon_link_args UNIX_COMMAND "${_pylon_link_flags}")
        foreach(_arg IN LISTS _pylon_link_args)
            if(_arg MATCHES "^-L(.+)")
                list(APPEND Pylon_LIBRARY_DIRS "${CMAKE_MATCH_1}")
            elseif(_arg MATCHES "^-l.+")
                list(APPEND Pylon_LIBRARIES "${_arg}")
            elseif(_arg MATCHES "^-Wl,.+")
                list(APPEND Pylon_LINK_OPTIONS "${_arg}")
            elseif(IS_ABSOLUTE "${_arg}" AND EXISTS "${_arg}")
                list(APPEND Pylon_LIBRARIES "${_arg}")
            else()
                list(APPEND Pylon_LINK_OPTIONS "${_arg}")
            endif()
        endforeach()
    endif()
endif()

list(REMOVE_DUPLICATES Pylon_INCLUDE_DIRS)
list(REMOVE_DUPLICATES Pylon_LIBRARY_DIRS)
list(REMOVE_DUPLICATES Pylon_LIBRARIES)
list(REMOVE_DUPLICATES Pylon_LINK_OPTIONS)

include(FindPackageHandleStandardArgs)
if(WIN32)
    find_package_handle_standard_args(Pylon
        REQUIRED_VARS Pylon_INCLUDE_DIRS Pylon_LIBRARY
        VERSION_VAR Pylon_VERSION)
else()
    find_package_handle_standard_args(Pylon
        REQUIRED_VARS Pylon_CONFIG Pylon_INCLUDE_DIRS Pylon_LIBRARIES
        VERSION_VAR Pylon_VERSION)
endif()

if(Pylon_FOUND AND NOT TARGET Pylon::Pylon)
    add_library(Pylon::Pylon INTERFACE IMPORTED)
    set_target_properties(Pylon::Pylon PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Pylon_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${Pylon_LIBRARIES}")
    if(Pylon_LIBRARY_DIRS)
        set_property(TARGET Pylon::Pylon PROPERTY
            INTERFACE_LINK_DIRECTORIES "${Pylon_LIBRARY_DIRS}")
    endif()
    if(Pylon_LINK_OPTIONS)
        set_property(TARGET Pylon::Pylon PROPERTY
            INTERFACE_LINK_OPTIONS "${Pylon_LINK_OPTIONS}")
    endif()
endif()

# Back-compat aliases for older CMake code and local scripts.
set(PYLON_FOUND "${Pylon_FOUND}")
set(PYLON_INCLUDE_DIRS "${Pylon_INCLUDE_DIRS}")
set(PYLON_LIBRARIES "${Pylon_LIBRARIES}")
set(PYLON_VERSION "${Pylon_VERSION}")
set(PYLON_BASE_LIBRARY "${Pylon_BASE_LIBRARY}")

mark_as_advanced(
    Pylon_INCLUDE_DIR
    Pylon_LIBRARY
    Pylon_BASE_LIBRARY
    Pylon_GCBASE_LIBRARY
    Pylon_GENAPI_LIBRARY
    Pylon_UTILITY_LIBRARY
    Pylon_CONFIG)
