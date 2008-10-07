; Script generated by the HM NIS Edit Script Wizard.

; Compiler-defines to generate different types of installers
;   SP_XTA - Include TA units and textures
;   SP_ALLMAPS - Include all maps instead of just one
;   SP_CORE - No maps or ta content
;!define SP_XTA
;!define SP_ALLMAPS

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "TA Spring"
!define PRODUCT_VERSION "0.40pre1"
!define PRODUCT_PUBLISHER "The TA Spring team"
!define PRODUCT_WEB_SITE "http://taspring.clan-sy.com"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\SpringClient.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "gpl.txt"
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
;!define MUI_FINISHPAGE_RUN "$INSTDIR\spring.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\docs\readme.html"
!define MUI_FINISHPAGE_LINK "The TA Spring website"
!define MUI_FINISHPAGE_LINK_LOCATION "http://taspring.clan-sy.com"
!define MUI_FINISHPAGE_TEXT "${PRODUCT_NAME} version ${PRODUCT_VERSION} has been successfully installed on your computer. It is recommended that you configure TA Spring settings now if this is a fresh installation, otherwise you may encounter problems."
!define MUI_FINISHPAGE_RUN "$INSTDIR\settings.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Configure ${PRODUCT_NAME} settings now"
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"

; Determine a suitable output name
!define SP_BASENAME "taspring_${PRODUCT_VERSION}"

!ifndef SP_ALLMAPS
!define SP_OUTSUFFIX1 "_small"
!else
!define SP_OUTSUFFIX1 ""
!endif

!ifndef SP_XTA
!define SP_OUTSUFFIX2 "_no_ta_content"
!else
!define SP_OUTSUFFIX2 ""
!endif

; Special handling for a "core" install
!ifdef SP_CORE
!ifdef SP_OUTSUFFIX1
!undef SP_OUTSUFFIX1
!endif
!ifdef SP_OUTSUFFIX2
!undef SP_OUTSUFFIX2
!endif
!define SP_OUTSUFFIX1 "_core"
!define SP_OUTSUFFIX2 ""
!endif

;OutFile "Setup.exe"
OutFile "${SP_BASENAME}${SP_OUTSUFFIX1}${SP_OUTSUFFIX2}.exe"
InstallDir "$PROGRAMFILES\TASpring"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

!include checkdotnet.nsh

; Used to make sure that the desktop icon to the battleroom cannot be installed without the battleroom itself
Function .onSelChange
  Push $0
  Push $1

  ; Determine which section to affect
!ifdef SP_CORE
  Push 3
!else
!ifdef SP_XTA
  Push 5
!else
  Push 4
!endif
!endif
  Pop $1
  
  SectionGetFlags 1 $0
  IntOp $0 $0 & 1
  IntCmp $0 0 NoBattle Battle Battle
  
  ; Battleroom is enabled
  Battle:
    SectionGetFlags $1 $0
    IntOp $0 $0 & 15
    SectionSetFlags $1 $0
    
    ; Make sure .NET is available
    Call IsDotNETInstalled
    Pop $0
    StrCmp $0 1 Found NotFound
    
    NotFound:
     MessageBox MB_ICONINFORMATION|MB_OK "The multiplayer battleroom requires the Microsoft .NET 1.1 framework to be installed. It does not appear that you have this installed though. You can obtain this through Windows Update or by visiting the Microsoft website."

    Found:
    
    Goto Done

  ; Battleroom is disabled
  NoBattle:
    SectionGetFlags $1 $0
    IntOp $0 $0 & 14
    IntOp $0 $0 | 16
    SectionSetFlags $1 $0
    Goto Done

  ; Done doing battleroom stuff
  Done:
  
  Pop $1
  Pop $0
FunctionEnd

; If .net is not installed, deselects the lobby as default
Function .onInit
  Push $0

  Call IsDotNETInstalled
  Pop $0
  StrCmp $0 1 Found NotFound

  NotFound:
    SectionGetFlags 1 $0
    IntOp $0 $0 & 14
    SectionSetFlags 1 $0
  Found:
   
  ; The core cannot be deselected
  SectionGetFlags 0 $0
;  IntOp $0 $0 & 14
  IntOp $0 $0 | 16
  SectionSetFlags 0 $0
  
  Pop $0

FunctionEnd

Section "Main application (req)" SEC_MAIN
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  
  ; Main stuff
  File "..\rts\bagge\spring.exe"
  File "..\rts\bagge\armor.txt"
  File "..\rts\bagge\bagge.fnt"
  File "..\rts\bagge\hpiutil.dll"
  File "..\rts\bagge\luxi.ttf"
  File "..\rts\bagge\selectioneditor.exe"
  File "..\rts\bagge\selectkeys.txt"
  File "..\rts\bagge\uikeys.txt"
  File "..\rts\bagge\settings.exe"
  File "..\rts\bagge\zlib.dll"
  File "..\rts\bagge\crashrpt.dll"
  File "..\rts\bagge\dbghelp.dll"
  File "..\rts\bagge\tower.sdu"
  File "..\rts\bagge\palette.pal"
  
  ; Bitmaps that are not from TA
  SetOutPath "$INSTDIR\bitmaps"
  File "..\rts\bagge\bitmaps\*.gif"
  File "..\rts\bagge\bitmaps\*.bmp"
  File "..\rts\bagge\bitmaps\*.jpg"

  SetOutPath "$INSTDIR\bitmaps\smoke"
  File "..\rts\bagge\bitmaps\smoke\*.bmp"
  
  SetOutPath "$INSTDIR\bitmaps\terrain"
  File "..\rts\bagge\bitmaps\terrain\*.jpg"
  
  SetOutPath "$INSTDIR\shaders"
  File "..\rts\bagge\shaders\*.fp"
  File "..\rts\bagge\shaders\*.vp"
  
  SetOutPath "$INSTDIR\aidll"
  File "..\rts\bagge\aidll\centralbuild.dll"
  File "..\rts\bagge\aidll\mmhandler.dll"
  File "..\rts\bagge\aidll\simpleform.dll"
  
SectionEnd

Section "Multiplayer battleroom" SEC_BATTLEROOM
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; The battleroom
  File "..\rts\bagge\ClientControls.dll"
  File "..\rts\bagge\SpringClient.exe"
  File "..\rts\bagge\Utility.dll"
  File "..\rts\bagge\Unitsync.dll"
SectionEnd

!ifndef SP_CORE
Section "Maps" SEC_MAPS
  SetOutPath "$INSTDIR\maps"
  SetOverwrite ifnewer

  File "..\rts\bagge\maps\Small Divide.sm2"
  File "..\rts\bagge\maps\Small Divide.smd"
!ifdef SP_XTA
  File "..\rts\bagge\maps\Small Divide.pe2"
  File "..\rts\bagge\maps\Small Divide.pe"
!endif

!ifdef SP_ALLMAPS
  File "..\rts\bagge\maps\FloodedDesert.sm2"
  File "..\rts\bagge\maps\FloodedDesert.smd"
!ifdef SP_XTA
  File "..\rts\bagge\maps\FloodedDesert.pe2"
  File "..\rts\bagge\maps\FloodedDesert.pe"
!endif

  File "..\rts\bagge\maps\Mars.sm2"
  File "..\rts\bagge\maps\Mars.smd"
!ifdef SP_XTA
  File "..\rts\bagge\maps\Mars.pe2"
  File "..\rts\bagge\maps\Mars.pe"
!endif

  File "..\rts\bagge\maps\map4.sm2"
  File "..\rts\bagge\maps\map4.smd"
!ifdef SP_XTA
  File "..\rts\bagge\maps\map4.pe2"
  File "..\rts\bagge\maps\map4.pe"
!endif

!endif
SectionEnd
!endif

!ifdef SP_XTA
Section "XTA for Spring" SEC_XTA
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  File "..\rts\bagge\taenheter.ccx"
  
SectionEnd
!endif

Section "Start menu shortcuts" SEC_START
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; Main shortcuts
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
;  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\TA Spring.lnk" "$INSTDIR\spring.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\TA Spring battleroom.lnk" "$INSTDIR\SpringClient.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk" "$INSTDIR\SelectionEditor.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk" "$INSTDIR\Settings.exe"

  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section /o "Desktop shortcut" SEC_DESKTOP
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  CreateShortCut "$DESKTOP\TA Spring battleroom.lnk" "$INSTDIR\SpringClient.exe"
SectionEnd

Section -Documentation
  SetOutPath "$INSTDIR\docs"
  SetOverwrite ifnewer
  
  File "..\readme.html"
  File "..\license.html"
  
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\springclient.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\spring.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "The core components required to run TA Spring. This includes the configuration utilities.$\n$\nNote: This section is required and cannot be deselected."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_BATTLEROOM} "The multiplayer battleroom used to set up multiplayer games and find opponents.$\n$\nNote: You need the Microsoft .NET 1.1 framework installed to use the battleroom."

!ifndef SP_CORE
!ifdef SP_ALLMAPS
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAPS} "Includes four maps to play TA Spring with.$\n$\nThese maps are called Small Divide, Flooded Desert, Mars and Map04."
!else
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAPS} "Includes one map to play TA Spring with.$\n$\nThe map is called Small Divide."
!endif
!endif

!ifdef SP_XTA
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_XTA} "Installs XTA which provides balanced units to play the game with.$\n$\nNote that this includes content from the game Total Annihilation, so only install this if you own the game."
!endif

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_START} "This creates shortcuts on the start menu to all the applications provided."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "This creates a shortcut on the desktop to the multiplayer battleroom for quick access to multiplayer games."

!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall

  ; Main files
  Delete "$INSTDIR\spring.exe"
  Delete "$INSTDIR\armor.txt"
  Delete "$INSTDIR\bagge.fnt"
  Delete "$INSTDIR\hpiutil.dll"
  Delete "$INSTDIR\luxi.ttf"
  Delete "$INSTDIR\palette.pal"
  Delete "$INSTDIR\selectioneditor.exe"
  Delete "$INSTDIR\selectkeys.txt"
  Delete "$INSTDIR\uikeys.txt"
  Delete "$INSTDIR\settings.exe"
  Delete "$INSTDIR\zlib.dll"
  Delete "$INSTDIR\crashrpt.dll"
  Delete "$INSTDIR\dbghelp.dll"
  Delete "$INSTDIR\tower.sdu"
  Delete "$INSTDIR\palette.pal"

  ; Bitmaps that are not from TA
  Delete "$INSTDIR\bitmaps\*.gif"
  Delete "$INSTDIR\bitmaps\*.bmp"
  Delete "$INSTDIR\bitmaps\*.jpg"
  Delete "$INSTDIR\bitmaps\smoke\*.bmp"
  Delete "$INSTDIR\bitmaps\terrain\*.jpg"
  RMDir "$INSTDIR\bitmaps\smoke"
  RMDir "$INSTDIR\bitmaps\terrain"
  RMDir "$INSTDIR\bitmaps"

  ; Shaders
  Delete "$INSTDIR\shaders\*.fp"
  Delete "$INSTDIR\shaders\*.vp"
  RMDir "$INSTDIR\shaders"
  
  ; AI-dll's
  Delete "$INSTDIR\aidll\centralbuild.dll"
  Delete "$INSTDIR\aidll\mmhandler.dll"
  Delete "$INSTDIR\aidll\simpleform.dll"

  ; The battleroom
  Delete "$INSTDIR\ClientControls.dll"
  Delete "$INSTDIR\SpringClient.exe"
  Delete "$INSTDIR\Utility.dll"
  Delete "$INSTDIR\Unitsync.dll"

  ; Maps
  Delete "$INSTDIR\maps\Small Divide.*"
!ifdef SP_ALLMAPS
  Delete "$INSTDIR\maps\FloodedDesert.*"
  Delete "$INSTDIR\maps\Mars.*"
  Delete "$INSTDIR\maps\map4.*"
!endif
  RMDir "$INSTDIR\maps"

  ; XTA
!ifdef SP_XTA
  Delete "$INSTDIR\taenheter.ccx"
!endif

  ; Generated stuff from the installer
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  
  ; Generated stuff from running spring
  Delete "$INSTDIR\infolog.txt"
  Delete "$INSTDIR\ext.txt"
  Delete "$INSTDIR\test.sdf"

  ; Documentation
  Delete "$INSTDIR\docs\readme.html"
  Delete "$INSTDIR\docs\license.html"
  RMDir "$INSTDIR\docs"

  ; Shortcuts
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk"
;  Delete "$SMPROGRAMS\${PRODUCT_NAME}\TA Spring.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\TA Spring battleroom.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  Delete "$DESKTOP\TA Spring battleroom.lnk"

  ; All done
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
