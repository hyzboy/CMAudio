macro(CMAudioSetup CMAUDIO_ROOT_PATH)

    message("CMAUDIO_ROOT_PATH: " ${CMAUDIO_ROOT_PATH})

    set(CMAUDIO_ROOT_INCLUDE_PATH    ${CMAUDIO_ROOT_PATH}/inc)
    set(CMAUDIO_ROOT_SOURCE_PATH     ${CMAUDIO_ROOT_PATH}/src)

    include_directories(${CMAUDIO_ROOT_INCLUDE_PATH})
endmacro()

# Define add_cm_library macro for CMAudio build system
# This creates a static library with the given name and sources
macro(add_cm_library TARGET_NAME PREFIX)
    # Parse additional arguments (source files)
    set(SOURCE_FILES ${ARGN})
    
    # Create the library target
    add_library(${PREFIX}.${TARGET_NAME}.Static STATIC ${SOURCE_FILES})
    
    # Set library properties
    set_target_properties(${PREFIX}.${TARGET_NAME}.Static PROPERTIES
        OUTPUT_NAME ${TARGET_NAME}
        POSITION_INDEPENDENT_CODE ON
    )
endmacro()

# Define add_cm_plugin macro for plugin build system
# This creates a shared/module library for plugins
# Usage: add_cm_plugin(TARGET_NAME PREFIX source1.cpp source2.cpp ... [lib1 lib2 ...])
macro(add_cm_plugin TARGET_NAME PREFIX)
    # Separate source files from library names
    set(PLUGIN_SOURCES)
    set(PLUGIN_LIBS)
    
    foreach(ARG ${ARGN})
        # Check if this looks like a source file
        if(ARG MATCHES "\\.(c|cpp|cxx|cc|C)$")
            list(APPEND PLUGIN_SOURCES ${ARG})
        else()
            # It's likely a library name or a non-source file, treat as source first
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${ARG})
                list(APPEND PLUGIN_SOURCES ${ARG})
            else()
                # Assume it's a library name
                list(APPEND PLUGIN_LIBS ${ARG})
            endif()
        endif()
    endforeach()
    
    # Create the plugin library target
    if(PLUGIN_SOURCES)
        add_library(${TARGET_NAME} MODULE ${PLUGIN_SOURCES})
        
        # Link libraries if any
        if(PLUGIN_LIBS)
            target_link_libraries(${TARGET_NAME} PRIVATE ${PLUGIN_LIBS})
        endif()
        
        # Set library properties
        set_target_properties(${TARGET_NAME} PROPERTIES
            PREFIX ""
            POSITION_INDEPENDENT_CODE ON
        )
    else()
        message(WARNING "No source files found for plugin ${TARGET_NAME}")
    endif()
endmacro()
