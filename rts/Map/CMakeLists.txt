
# This list was created using this *nix shell command:
# > find . -name "*.cpp"" | sort
set(sources_engine_Map
		"${CMAKE_CURRENT_SOURCE_DIR}/BaseGroundDrawer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/BasicMapDamage.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Ground.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/HeightLinePalette.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/HeightMapTexture.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/MapDamage.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/MapInfo.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/MapParser.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/MetalMap.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/ReadMap.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Generation/MapGenerator.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Generation/SimpleMapGenerator.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/SMFGroundDrawer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/SMFGroundTextures.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/SMFMapFile.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/SMFReadMap.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/SMFRenderState.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/Basic/BasicMeshDrawer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/Legacy/LegacyMeshDrawer.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/ROAM/Patch.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/SMF/ROAM/RoamMeshDrawer.cpp"
		PARENT_SCOPE
	)
