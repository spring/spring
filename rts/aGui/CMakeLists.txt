
# This list was created using this *nix shell command:
# > find . -name "*.cpp"" | sort
SET(sources_engine_aGui
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

MakeGlobal(sources_engine_aGui)
