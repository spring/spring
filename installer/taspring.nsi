; Script generated by the HM NIS Edit Script Wizard.

; Compiler-defines to generate different types of installers
;   SP_UPDATE - Only include changed files and no maps
;   SP_PATCH - Creates a very small patching file (typically from just latest version)

; Use the 7zip-like compressor
SetCompressor lzma

!include "springsettings.nsh"

; HM NIS Edit Wizard helper defines
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
; Licensepage
!insertmacro MUI_PAGE_LICENSE "gpl.txt"

!ifndef SP_PATCH
; Components page
!insertmacro MUI_PAGE_COMPONENTS
!endif

; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES

; Finish page
!ifndef SP_PATCH

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\docs\main.html"
!define MUI_FINISHPAGE_TEXT "${PRODUCT_NAME} version ${PRODUCT_VERSION} has been successfully installed on your computer. It is recommended that you configure TA Spring settings now if this is a fresh installation, otherwise you may encounter problems."

!define MUI_FINISHPAGE_RUN "$INSTDIR\settings.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Configure ${PRODUCT_NAME} settings now"

!else

!define MUI_FINISHPAGE_TEXT "${PRODUCT_NAME} version ${PRODUCT_VERSION} has been successfully updated from a previous version."

!endif

!define MUI_FINISHPAGE_LINK "The ${PRODUCT_NAME} website"
!define MUI_FINISHPAGE_LINK_LOCATION ${PRODUCT_WEB_SITE}
!define MUI_FINISHPAGE_NOREBOOTSUPPORT

!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"

!ifdef SP_UPDATE
!define SP_OUTSUFFIX1 "_update"
!else
!ifdef SP_PATCH
!define SP_OUTSUFFIX1 "_patch"
!else
!define SP_OUTSUFFIX1 ""
!endif
!endif

OutFile "${SP_BASENAME}${SP_OUTSUFFIX1}.exe"
InstallDir "$PROGRAMFILES\TASpring"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

!include fileassoc.nsh

!ifndef SP_PATCH

; Used to make sure that the desktop icon to the battleroom cannot be installed without the battleroom itself
Function .onSelChange
  Push $0
  Push $1

  ; Determine which section to affect (since UPDATE do not have a map section)
!ifdef SP_UPDATE
  Push 3
!else
  Push 4
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

Function .onInit
  Push $0

  ; The core cannot be deselected
  SectionGetFlags 0 $0
;  IntOp $0 $0 & 14
  IntOp $0 $0 | 16
  SectionSetFlags 0 $0
  
  Pop $0

FunctionEnd

Function CheckMaps
  FindFirst $0 $1 "$INSTDIR\maps\*.sm2"
  StrCmp $1 "" done

  MessageBox MB_ICONQUESTION|MB_YESNOCANCEL "The installer has detected old maps in the destination folder. This version of Spring uses a new and better, but incompatible, map format. This means that the old maps can no longer be used, and must be removed to prevent conflicts. Would you like to delete the current contents of your maps folder? If you answer no, the contents will be copied to a new folder called 'oldmaps' to prevent conflicts. You can then delete them manually later. Press cancel to abort the installation." IDYES delete IDNO rename
    Abort "Installation aborted"
    Goto done
  delete:
    Delete "$INSTDIR\maps\*.*"
    Goto done
  rename:
    CreateDirectory "$INSTDIR\oldmaps"
    CopyFiles "$INSTDIR\maps\*.*" "$INSTDIR\oldmaps"
    Delete "$INSTDIR\maps\*.*"
    Goto done
  done:
FunctionEnd

; Deletes spawn.txt if it is from an original installation
Function UpdateSpawn
  ClearErrors
  FileOpen $0 "$INSTDIR\spawn.txt" r
  IfErrors done
  FileSeek $0 0 END $1
  IntCmp $1 260 Eq Less Eq
  
Less:
  FileClose $0
  Delete "$INSTDIR\spawn.txt"
  Goto done
Eq:
  FileClose $0
  Goto done

Done:
FunctionEnd

; Only allow installation if spring.exe is from version 0.67bx
Function CheckVersion
  ClearErrors
  FileOpen $0 "$INSTDIR\spring.exe" r
  IfErrors done
  FileSeek $0 0 END $1
;  IntCmp $1 2637824 Done             ; 0.60b1
;  IntCmp $1 2650112 Done             ; 0.61b1
;  IntCmp $1 2670592 Done             ; 0.61b2
;  IntCmp $1 2678784 Done             ; 0.62b1
;  IntCmp $1 2682880 Done             ; 0.63b1 & 0.63b2
;  IntCmp $1 2703360 Done             ; 0.64b1
;  IntCmp $1 3006464 Done             ; 0.65b1
;  IntCmp $1 3014656 Done             ; 0.65b2
;  IntCmp $1 3031040 Done             ; 0.66b1
;  IntCmp $1 3035136 Done             ; 0.67b1 & 0.67b2 & 0.67b3
  IntCmp $1 2633728 Done             ; 0.70b1
  IntCmp $1 2650112 Done			 ; 0.70b2
  IntCmp $1 2707456 Done			 ; 0.70b3
  MessageBox MB_ICONSTOP|MB_OK "This installer can only be used to upgrade a full installation of TA Spring 0.70b*. Your current folder does not contain a spring.exe from any such version, so the installation will be aborted.. Please download the full installer instead and try again."
  Abort "Unable to upgrade, version 0.70b1, 0.70b2 or 0.70b3 not found.."
  Goto done

Done:
  FileClose $0

FunctionEnd

Section "Main application (req)" SEC_MAIN
  SetOutPath "$INSTDIR"

!ifdef SP_UPDATE
  Call CheckVersion
!endif
  Call CheckMaps
  Call UpdateSpawn

  !define INSTALL
  !include "sections\main.nsh"
  !undef INSTALL
SectionEnd


!ifndef NIGHTLY_BUILD
Section "Multiplayer battleroom" SEC_BATTLEROOM
  !define INSTALL
  !include "sections\battleroom.nsh"
  !undef INSTALL
SectionEnd
!endif


!ifndef SP_UPDATE
Section "Maps" SEC_MAPS
  !define INSTALL
  !include "sections\maps.nsh"
  !undef INSTALL
SectionEnd
!endif

Section "Start menu shortcuts" SEC_START
  !define INSTALL
  !include "sections\shortcuts.nsh"
  !undef INSTALL
SectionEnd

!ifndef NIGHTLY_BUILD
Section /o "Desktop shortcut" SEC_DESKTOP
  SetOutPath "$INSTDIR"

  CreateShortCut "$DESKTOP\${PRODUCT_NAME} battleroom.lnk" "$INSTDIR\TASClient.exe"
SectionEnd
!endif

SectionGroup "AI opponent plugins (Bots)"
	Section "AAI" SEC_AAI
	!define INSTALL
	!include "sections\aai.nsh"
	!undef INSTALL
	SectionEnd
SectionGroupEnd

!include "sections\sectiondesc.nsh"

!else ; SP_PATCH

; Only allow installation if spring.exe is from version 0.70b1
Function CheckVersion
  ClearErrors
  FileOpen $0 "$INSTDIR\tasclient.exe" r
  IfErrors done
  FileSeek $0 0 END $1
;  IntCmp $1 3035136 Done             ; 0.67b1 &0.67b2
;  IntCmp $1 2277888 Done             ; 0.67b2 (tasclient 0.19)
  IntCmp $1 2640896 Done              ; 0.70b1 (tasclient 0.20)

  MessageBox MB_ICONSTOP|MB_OK "This installer can only be used to patch a full installation of TA Spring 0.70b1. Your current folder does not contain a tasclient.exe from this version, so the installation will be aborted.. Please download the full or updating installer instead and try again."
  Abort "Unable to upgrade, version 0.70b1 not found.."
  Goto done

Done:
  FileClose $0

FunctionEnd

!include "VPatchLib.nsh"

Section -Patch
  SetOutPath "$INSTDIR"
  Call CheckVersion
  
  !insertmacro VPatchFile "tasclient070b2.pat" "$INSTDIR\TASClient.exe" "$TEMP\TASClient.tmp"
  IfErrors PatchError
  !insertmacro VPatchFile "spring070b2.pat" "$INSTDIR\spring.exe" "$TEMP\spring.tmp"
  IfErrors PatchError
  !insertmacro VPatchFile "unitsync070b2.pat" "$INSTDIR\unitsync.dll" "$TEMP\unitsync.tmp"
  IfErrors PatchError
  !insertmacro VPatchFile "sdl070b2.pat" "$INSTDIR\sdl.dll" "$TEMP\sdl.tmp"
  IfErrors PatchError

;  !insertmacro VPatchFile "unitsync067b2.pat" "$INSTDIR\unitsync.dll" "$TEMP\unitsync.tmp"
;  IfErrors PatchError

  Goto Done

PatchError:
  MessageBox MB_ICONSTOP|MB_OK "The patching process could not be completed. Please download the full or updating installer instead and install one of them instead."
  Abort "Error encountered during patching.."
  
Done:

SectionEnd

!endif ; SP_PATCH

Section -Documentation
  !define INSTALL
  !include "sections\docs.nsh"
  !undef INSTALL
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

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall

  !include "sections\maps.nsh"
  !include "sections\main.nsh"

  !include "sections\docs.nsh"
  !include "sections\shortcuts.nsh"
  !include "sections\aai.nsh"
  !include "sections\battleroom.nsh"

  Delete "$DESKTOP\TA Spring battleroom.lnk"

  ; All done
  RMDir "$INSTDIR"

  !insertmacro APP_UNASSOCIATE "sdf" "taspring.demofile"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
