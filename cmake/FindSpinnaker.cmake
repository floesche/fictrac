# FindSpinnaker.cmake
#
# Locates the FLIR / Teledyne / Point Grey Spinnaker SDK (used to capture
# from PGR USB3 / FLIR cameras). Spinnaker does not ship a CMake config
# file, so this module searches the standard install locations.
#
# Hints (any of these are honoured if set):
#   - Spinnaker_ROOT (cache var or env, recommended; CMake 3.12+ uses it
#     automatically as a search prefix)
#   - SPINNAKER_ROOT (cache var or env, alias)
#   - PGR_DIR        (cache var or env, legacy name kept for back-compat)
#
# Result variables:
#   Spinnaker_FOUND        - true if SDK was found
#   Spinnaker_INCLUDE_DIRS - header search path
#   Spinnaker_LIBRARIES    - library to link
#   Spinnaker_VERSION      - SDK version (e.g. "4.2.0.83") if discoverable
#
# Imported target:
#   Spinnaker::Spinnaker

set(_spinnaker_hints
    ${SPINNAKER_ROOT}
    ${PGR_DIR}
    $ENV{SPINNAKER_ROOT}
    $ENV{PGR_DIR})

if(WIN32)
    set(_spinnaker_default_paths
        "C:/Program Files/Point Grey Research/Spinnaker"
        "C:/Program Files/FLIR Systems/Spinnaker"
        "C:/Program Files/Teledyne/Spinnaker")
    set(_spinnaker_inc_suffixes  include)
    set(_spinnaker_lib_suffixes  lib64/vs2015 lib64/vs2017 lib64/vs2019 lib64/vs2022)
    # Spinnaker's Windows lib is suffixed with the MSVC toolset version it
    # was built against. Try the common ones in newest-first order.
    set(_spinnaker_lib_names
        Spinnaker_v143 Spinnaker_v142 Spinnaker_v141 Spinnaker_v140)
else()
    set(_spinnaker_default_paths
        /opt/spinnaker
        /opt/flir/spinnaker
        /usr/local
        /usr)
    # Tarball installs put headers at <root>/include; .deb installs put
    # them under <root>/include/spinnaker.
    set(_spinnaker_inc_suffixes  include include/spinnaker)
    set(_spinnaker_lib_suffixes  lib lib64 lib/x86_64-linux-gnu)
    set(_spinnaker_lib_names     Spinnaker)
endif()

find_path(Spinnaker_INCLUDE_DIR
    NAMES Spinnaker.h
    HINTS ${_spinnaker_hints}
    PATHS ${_spinnaker_default_paths}
    PATH_SUFFIXES ${_spinnaker_inc_suffixes})

find_library(Spinnaker_LIBRARY
    NAMES ${_spinnaker_lib_names}
    HINTS ${_spinnaker_hints}
    PATHS ${_spinnaker_default_paths}
    PATH_SUFFIXES ${_spinnaker_lib_suffixes})

# Pull a version string out of System.h if we can find it
if(Spinnaker_INCLUDE_DIR AND EXISTS "${Spinnaker_INCLUDE_DIR}/System.h")
    file(STRINGS "${Spinnaker_INCLUDE_DIR}/System.h" _spinnaker_ver_lines
        REGEX "FLIR_SPINNAKER_VERSION_(MAJOR|MINOR|TYPE|BUILD)")
    foreach(_line IN LISTS _spinnaker_ver_lines)
        if(_line MATCHES "FLIR_SPINNAKER_VERSION_MAJOR[ \t]+([0-9]+)")
            set(_v_major "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "FLIR_SPINNAKER_VERSION_MINOR[ \t]+([0-9]+)")
            set(_v_minor "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "FLIR_SPINNAKER_VERSION_TYPE[ \t]+([0-9]+)")
            set(_v_type  "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "FLIR_SPINNAKER_VERSION_BUILD[ \t]+([0-9]+)")
            set(_v_build "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(DEFINED _v_major AND DEFINED _v_minor)
        set(Spinnaker_VERSION "${_v_major}.${_v_minor}.${_v_type}.${_v_build}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spinnaker
    REQUIRED_VARS Spinnaker_LIBRARY Spinnaker_INCLUDE_DIR
    VERSION_VAR   Spinnaker_VERSION)

if(Spinnaker_FOUND AND NOT TARGET Spinnaker::Spinnaker)
    add_library(Spinnaker::Spinnaker UNKNOWN IMPORTED)
    set_target_properties(Spinnaker::Spinnaker PROPERTIES
        IMPORTED_LOCATION             "${Spinnaker_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Spinnaker_INCLUDE_DIR}")
    set(Spinnaker_INCLUDE_DIRS "${Spinnaker_INCLUDE_DIR}")
    set(Spinnaker_LIBRARIES    "${Spinnaker_LIBRARY}")
endif()

mark_as_advanced(Spinnaker_INCLUDE_DIR Spinnaker_LIBRARY)
