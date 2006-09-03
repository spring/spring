
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  ; Main shortcuts
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Spring multiplayer battleroom.lnk" "$INSTDIR\TASClient.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk" "$INSTDIR\SelectionEditor.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk" "$INSTDIR\Settings.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Spring test.lnk" "$INSTDIR\spring.exe"

  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
!else
  ; Shortcuts
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring test.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring multiplayer battleroom.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"


!endif
