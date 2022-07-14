if (${${PROJECT_NAME}_SIM_MOUSE})
	add_subdirectory("mouse")
endif()

if (${${PROJECT_NAME}_SIM_ELEGANS})
	add_subdirectory("worm")
endif()
