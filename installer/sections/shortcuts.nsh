
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  ; Main shortcuts
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  ${If} ${SectionIsSelected} ${SEC_TASCLIENT}
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}\Multiplayer"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Multiplayer\TASClient.lnk" "$INSTDIR\TASClient.exe -server 82.239.138.44:8300"
  ${EndIf}
  ${If} ${SectionIsSelected} ${SEC_CA}
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Update CA.lnk" "$INSTDIR\CaDownloader.exe"
  ${EndIf}  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk" "$INSTDIR\SelectionEditor.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk" "$INSTDIR\Settings.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk" "$INSTDIR\spring.exe"

  WriteIniStr "$INSTDIR\Spring.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  WriteIniStr "$INSTDIR\unknown-files.url" "InternetShortcut" "URL" "http://spring.unknown-files.net"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Spring Website.lnk" "$INSTDIR\Spring.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Download Content.lnk" "$INSTDIR\$$INSTDIR\unknown-files.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk" "$INSTDIR\uninst.exe"
!else
  ; Shortcuts
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Multiplayer\TASClient.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring Website.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Download Content.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Update CA.lnk"
  
  ; delete the old shortcuts if they're present from a prior installation
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring test.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring multiplayer battleroom.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk"

  ; delete the .url files
  Delete "$INSTDIR\Spring.url"
  Delete "$INSTDIR\unknown-files.url"

  ; delete the folders
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}\Multiplayer"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

!endif
