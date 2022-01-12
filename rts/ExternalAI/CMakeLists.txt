
# This list was created using this *nix shell command:
# > find . -name "*.cpp"" | sort
set(sources_engine_ExternalAI
		"${CMAKE_CURRENT_SOURCE_DIR}/AICallback.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/AICheats.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/AIInterfaceKey.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/AIInterfaceLibrary.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/AIInterfaceLibraryInfo.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/AILibraryManager.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/AISCommands.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/EngineOutHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/LuaAIImplHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SAIInterfaceCallbackImpl.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SSkirmishAICallbackImpl.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SkirmishAIData.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SkirmishAIHandler.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SkirmishAIKey.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SkirmishAILibrary.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SkirmishAILibraryInfo.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SkirmishAIWrapper.cpp"
		PARENT_SCOPE
	)
