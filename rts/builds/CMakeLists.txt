
if    (NOT HEADLESS_SYSTEM)
	Add_Subdirectory(default)
	Add_Subdirectory(GML)
	SET(EXCLUDE_HEADLESS_FROM_ALL EXCLUDE_FROM_ALL)
else  (NOT HEADLESS_SYSTEM)
	# build HL and DS with build target "all"
	SET(EXCLUDE_HEADLESS_FROM_ALL )
endif (NOT HEADLESS_SYSTEM)

Add_Subdirectory(DS)
Add_Subdirectory(HL)

