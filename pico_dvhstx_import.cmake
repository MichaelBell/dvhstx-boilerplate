# This file can be dropped into a project to help locate the Picovision library
# It will also set up the required include and module search paths.

if (NOT PICO_DVHSTX_PATH)
    set(PICO_DVHSTX_PATH "../../dvhstx/")
endif()

if(NOT IS_ABSOLUTE ${PICO_DVHSTX_PATH})
    get_filename_component(
        PICO_DVHSTX_PATH
        "${CMAKE_CURRENT_BINARY_DIR}/${PICO_DVHSTX_PATH}"
        ABSOLUTE)
endif()

if (NOT EXISTS ${PICO_DVHSTX_PATH})
    message(FATAL_ERROR "Directory '${PICO_DVHSTX_PATH}' not found")
endif ()

if (NOT EXISTS ${PICO_DVHSTX_PATH}/pico_dvhstx.cmake)
    message(FATAL_ERROR "Directory '${PICO_DVHSTX_PATH}' does not appear to contain the DVHSTX library")
endif ()

message("PICO_DVHSTX_PATH is ${PICO_DVHSTX_PATH}")

set(PICO_DVHSTX_PATH ${PICO_DVHSTX_PATH} CACHE PATH "Path to the DVHSTX libraries" FORCE)

include_directories(${PICO_DVHSTX_PATH})
list(APPEND CMAKE_MODULE_PATH ${PICO_DVHSTX_PATH})

include(pico_dvhstx)
include_directories(${PIMORONI_PICO_PATH}/libraries/pico_graphics)
