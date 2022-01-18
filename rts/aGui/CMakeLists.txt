add_library(engineaGui STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/Button.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Gui.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/GuiElement.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/HorizontalLayout.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/LineEdit.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/List.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Picture.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/TextElement.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/VerticalLayout.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Window.cpp"
	)
target_include_directories(engineaGui
	PRIVATE ${SDL2_INCLUDE_DIR}
	PRIVATE ${GLEW_INCLUDE_DIR}
	PRIVATE ${CMAKE_SOURCE_DIR}/rts/lib/slimsig/include)
