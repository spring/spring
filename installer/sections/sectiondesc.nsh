; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "The core components required to run Spring. This includes the configuration utilities.$\n$\nNote: This section is required and cannot be deselected."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_BATTLEROOM} "The multiplayer battleroom used to set up multiplayer games and find opponents.$\n$\nNote: This section is highly recommend and should generally not be deselected."

!ifndef SP_UPDATE
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAPS} "Includes three maps to play Spring with.$\n$\nThese maps are called Small Divide, Mars and Islands in War."
!endif

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_ARCHIVEMOVER} "The tool and the necessary file associations (for sd7 and sdz files) to ease installation of new content."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_AAI} "Includes AAI, an AI opponent made by submarine."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_KAI} "Includes KAI, an AI opponent originally made by Krogothe, Tournesol, now maintained by Kloot."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_START} "This creates shortcuts on the start menu to all the applications provided."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "This creates a shortcut on the desktop to the multiplayer battleroom for quick access to multiplayer games."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_BA} "Balanced Annihilation is a mod of Total Annihilation, and is currently the most popular game."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_XTA} "XTA is a mod of Total Annihilation, with a small but devoted following."

!insertmacro MUI_FUNCTION_DESCRIPTION_END
