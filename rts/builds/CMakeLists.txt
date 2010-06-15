
if    (NOT HEADLESS_SYSTEM)
	Add_Subdirectory(default)
	Add_Subdirectory(GML)
endif (NOT HEADLESS_SYSTEM)

Add_Subdirectory(DS)
Add_Subdirectory(HL)

