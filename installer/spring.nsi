; Script generated by the HM NIS Edit Script Wizard.

!addPluginDir "nsis_plugins"

; Use the 7zip-like compressor
SetCompressor /FINAL /SOLID lzma


!include "springsettings.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"
!include "WordFunc.nsh"
!insertmacro VersionCompare

; this registry entry is deprecated (march 2011, use HKLM\Software\Spring\SpringEngine[Helper] instead)
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\spring.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_ROOT_KEY "HKLM"
!define PRODUCT_KEY "Software\Spring"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "graphics\InstallerIcon.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "graphics\SideBanner.bmp"
;!define MUI_COMPONENTSPAGE_SMALLDESC ;puts description on the bottom, but much shorter.
!define MUI_COMPONENTSPAGE_TEXT_TOP "Some of these components must be downloaded during the install process."


; Welcome page
!insertmacro MUI_PAGE_WELCOME
; Licensepage
!insertmacro MUI_PAGE_LICENSE "..\gpl-2.0.txt"

; Components page
!insertmacro MUI_PAGE_COMPONENTS

; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES

; Finish page

!define MUI_FINISHPAGE_SHOWREADME "http://springrts.com/wiki/Read_Me_First"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open $\"Read Me First$\" Webpage"
;!define MUI_FINISHPAGE_RUN "$INSTDIR\spring.exe"
;!define MUI_FINISHPAGE_RUN_TEXT "Configure ${PRODUCT_NAME} settings now"
!define MUI_FINISHPAGE_TEXT "${PRODUCT_NAME} version ${PRODUCT_VERSION} has been successfully installed or updated from a previous version.  You should configure Spring settings now if this is a fresh installation.  If you did not install spring to C:\Program Files\Spring you will need to point the settings program to the install location."

!define MUI_FINISHPAGE_LINK "The Spring RTS Project website"
!define MUI_FINISHPAGE_LINK_LOCATION ${PRODUCT_WEB_SITE}
!define MUI_FINISHPAGE_NOREBOOTSUPPORT

!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"

!define SP_OUTSUFFIX1 ""

; set output filename for installer
OutFile "${SP_BASENAME}${SP_OUTSUFFIX1}.exe"
InstallDir "$PROGRAMFILES\Spring"
InstallDirRegKey ${PRODUCT_ROOT_KEY} "${PRODUCT_DIR_REGKEY}" "Path"

VAR REGISTRY ; if 1 registry values are written

!include "include\echo.nsh"
!include "include\fileassoc.nsh"
!include "include\fileExistChecks.nsh"
!include "include\fileMisc.nsh"
!include "include\extractFile.nsh"
!include "include\getParameterValue.nsh"


${!echonow} ""
${!echonow} "Base dir:   <engine-source-root>/installer/"

!ifndef MIN_PORTABLE_ARCHIVE
	!error "MIN_PORTABLE_ARCHIVE undefined: please specifiy where minimal-portable 7z-archive which contains the spring-engine is"
!else
	${!echonow} "Using MIN_PORTABLE_ARCHIVE: ${MIN_PORTABLE_ARCHIVE}"
!endif

!ifndef NSI_UNINSTALL_FILES
	!warning "NSI_UNINSTALL_FILES not defined"
!else
	${!echonow} "Using NSI_UNINSTALL_FILES:  ${NSI_UNINSTALL_FILES}"
!endif

!ifndef VCREDIST
	!error "VCREDIST not defined"
!else
	${!echonow} "Using VCREDIST:             ${VCREDIST}"
!endif


Section "Engine" SEC_MAIN
	; make this section read-only -> user can not deselect it
	SectionIn RO

	!define INSTALL
		${!echonow} "Processing: main"
		!include "sections\main.nsh"
		${!echonow} "Processing: deprecated"
	        !include "sections\deprecated.nsh"
	!undef INSTALL
SectionEnd

Section "Desktop shortcuts" SEC_DESKTOP
	!define INSTALL
		${!echonow} "Processing: shortcuts - Desktop"
		!include "sections\shortcuts_desktop.nsh"
	!undef INSTALL
SectionEnd

Section "Start menu shortcuts" SEC_START
	!define INSTALL
		${!echonow} "Processing: shortcuts - Start menu"
		!include "sections\shortcuts_startMenu.nsh"
	!undef INSTALL
SectionEnd


Section /o "Portable" SEC_PORTABLE
	!define INSTALL
		${!echonow} "Processing: Portable"
		!include "sections\portable.nsh"
	!undef INSTALL
SectionEnd

Section "" SEC_VCREDIST
	!define INSTALL
		${!echonow} "Processing: vcredist"
		!include "sections\vcredist.nsh"
	!undef INSTALL
SectionEnd

!include "sections\sectiondesc.nsh"

Section -Post
	${!echonow} "Processing: Registry entries"
	${IfNot} ${SectionIsSelected} ${SEC_PORTABLE}
		; make non-portable
		Delete "$INSTDIR\springsettings.cfg"
		; Create uninstaller
		WriteUninstaller "$INSTDIR\uninst.exe"
		${If} $REGISTRY = 1
			WriteRegStr ${PRODUCT_ROOT_KEY} "${PRODUCT_DIR_REGKEY}" "@" "$INSTDIR\spring.exe"
			WriteRegStr ${PRODUCT_ROOT_KEY} "${PRODUCT_DIR_REGKEY}" "Path" "$INSTDIR"
			WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
			WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
			WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\spring.exe"
			WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
			WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
			WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
		${EndIf}
	${EndIf}
SectionEnd

Function .onInit
	${!echonow} ""
	; enable/disable sections depending on parameters
	!include "sections/setupSections.nsh"
FunctionEnd

Function un.onUninstSuccess
	IfSilent +3
	HideWindow
	MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
	IfSilent +3
	MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
	Abort
FunctionEnd


Section Uninstall
	${!echonow} "Processing: Uninstall"

	!include "sections\main.nsh"
	!include "sections\deprecated.nsh"

	# FIXME workaround part2
	Delete "$INSTDIR\pthreadGC2.dll"

	!include "sections\shortcuts_startMenu.nsh"
	!include "sections\shortcuts_desktop.nsh"
	!include "sections\portable.nsh"
	!ifdef NSI_UNINSTALL_FILES
	!include "${NSI_UNINSTALL_FILES}"
	!endif
	; All done
	RMDir "$INSTDIR"

	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	DeleteRegKey ${PRODUCT_ROOT_KEY} "${PRODUCT_DIR_REGKEY}"
	SetAutoClose true
SectionEnd
