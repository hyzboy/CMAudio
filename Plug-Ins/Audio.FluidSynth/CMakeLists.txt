add_subdirectory(FluidLite)

add_cm_plugin(CMP.Audio.FluidSynth "Plug-Ins//Audio" MidiRead.cpp)

if(TARGET fluidlite-static)
	target_link_libraries(CMP.Audio.FluidSynth PRIVATE fluidlite-static)
elseif(TARGET fluidlite)
	target_link_libraries(CMP.Audio.FluidSynth PRIVATE fluidlite)
endif()