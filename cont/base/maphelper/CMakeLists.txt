set(FILES
	modinfo.lua
	MapOptions.lua
	maphelper/mapdefaults.lua
	maphelper/parse_tdf_map.lua
	maphelper/system.lua
	maphelper/applyopts.lua
	maphelper/parse_tdf.lua
	maphelper/setupopts.lua
	maphelper/mapinfo.lua
)

create_base_content_archive("base" "maphelper.sdz" "${FILES}")

