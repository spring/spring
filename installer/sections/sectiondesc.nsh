; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "The core components required to run Spring. This includes the configuration utilities.$\n$\nNote: This section is required and cannot be deselected."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_BATTLEROOM} "The multiplayer battleroom used to set up multiplayer games and find opponents.$\n$\nNote: This section is highly recommend and should generally not be deselected."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAPS} "Includes 3 default maps to play Spring with.These maps are called Small Divide, Comet Catcher Redux, and Sands of War. (9.0MB download)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_1V1MAPS} "A pack of 14 maps that are good for 1v1 play. (87.3MB download)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_TEAMMAPS} "A pack of 12 maps that are good for team play. (170.1MB download)"

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_ARCHIVEMOVER} "The tool and the necessary file associations (for sd7 and sdz files) to ease installation of new content."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_AAI} "Includes AAI, an AI opponent made by submarine."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_KAI} "Includes KAI, an AI opponent originally made by Krogothe, Tournesol, now maintained by Kloot."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_START} "This creates shortcuts on the start menu to all the applications provided."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "This creates a shortcut on the desktop to the multiplayer battleroom for quick access to multiplayer games."

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_BA} "Balanced Annihilation is a mod of Total Annihilation, and is currently the most popular game. Balanced Annihilation claims to have the most finely tuned balance of all the TA mods.  (10.4MB Download)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_XTA} "XTA is a mod of Total Annihilation originally developed by the Swedish Yankspankers.  It claims to have the most similar gameplay to the original game Total Annihilation (9.0MB Download)"

!insertmacro MUI_FUNCTION_DESCRIPTION_END
