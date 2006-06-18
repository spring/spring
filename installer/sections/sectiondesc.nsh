; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "The core components required to run TA Spring. This includes the configuration utilities.$\n$\nNote: This section is required and cannot be deselected."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_BATTLEROOM} "The multiplayer battleroom used to set up multiplayer games and find opponents."

!ifndef SP_UPDATE
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAPS} "Includes two maps to play TA Spring with.$\n$\nThese maps are called Small Divide and Mars."
!endif

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_AAI} "Includes AAI, a skirmish AI or 'bot', for newer versions visit the AI forum"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_NTAI} "Includes NTAI, another skirmish AI, for newer versions visit the AI forum"

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_START} "This creates shortcuts on the start menu to all the applications provided."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "This creates a shortcut on the desktop to the multiplayer battleroom for quick access to multiplayer games."

!insertmacro MUI_FUNCTION_DESCRIPTION_END

