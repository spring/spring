
# This list was created using this *nix shell command:
# > find . -name "*.cpp"" | sort
SET(sources_engine_Menu
		"${CMAKE_CURRENT_SOURCE_DIR}/LobbyConnection.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/ScriptHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SelectMenu.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SelectionWidget.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/UpdaterWindow.cpp"
	)

MakeGlobal(sources_engine_Menu)
