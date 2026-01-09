# FindPGR.cmake - Find the Spinnaker SDK (PGR/FLIR)
#
# Sets:
#   PGR_FOUND
#   PGR_INCLUDE_DIRS
#   PGR_LIBRARIES

find_path(PGR_INCLUDE_DIR
    NAMES Spinnaker.h
    HINTS
        $ENV{SPINNAKER_ROOT}/include
        ${SPINNAKER_ROOT}/include
        ${PGR_DIR}/include
        "C:/Program Files/FLIR Systems/Spinnaker/include"
        "C:/Program Files/Point Grey Research/Spinnaker/include"
        "C:/Program Files/Teledyne/Spinnaker/include"
        /usr/include
        /usr/local/include
)

find_library(PGR_LIBRARY
    NAMES Spinnaker_v140 Spinnaker
    HINTS
        $ENV{SPINNAKER_ROOT}/lib64/vs2015
        ${SPINNAKER_ROOT}/lib64/vs2015
        ${PGR_DIR}/lib64/vs2015
        "C:/Program Files/FLIR Systems/Spinnaker/lib64/vs2015"
        "C:/Program Files/Point Grey Research/Spinnaker/lib64/vs2015"
        "C:/Program Files/Teledyne/Spinnaker/lib64/vs2015"
        $ENV{SPINNAKER_ROOT}/lib64
        ${SPINNAKER_ROOT}/lib64
        ${PGR_DIR}/lib64
        "C:/Program Files/FLIR Systems/Spinnaker/lib64"
        "C:/Program Files/Point Grey Research/Spinnaker/lib64"
        "C:/Program Files/Teledyne/Spinnaker/lib64"
        /usr/lib
        /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PGR DEFAULT_MSG PGR_INCLUDE_DIR PGR_LIBRARY)

if(PGR_FOUND)
    set(PGR_INCLUDE_DIRS ${PGR_INCLUDE_DIR})
    set(PGR_LIBRARIES ${PGR_LIBRARY})
endif()
