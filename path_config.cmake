macro(CMAudioSetup CMAUDIO_ROOT_PATH)
    message("CMAudio root path: " ${CMAUDIO_ROOT_PATH})

    set(CMAUDIO_ROOT_INCLUDE_PATH ${CMAUDIO_ROOT_PATH}/inc)
    set(CMAUDIO_ROOT_SOURCE_PATH ${CMAUDIO_ROOT_PATH}/src)

    include_directories(${CMAUDIO_ROOT_INCLUDE_PATH})
endmacro()
