
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  SetOverWrite on

  ; Main stuff
  File "..\game\spring.exe"
  File "..\game\unitsync.dll"
  CreateDirectory "$INSTDIR\maps"
  CreateDirectory "$INSTDIR\mods"

  File "downloads\TASServer.jar"

  ; DLLs (updated in mingwlibs-v8)
  File "..\mingwlibs\dll\glew32.dll"
  File "..\mingwlibs\dll\python25.dll"
  File "..\mingwlibs\dll\zlib1.dll"
  File "..\mingwlibs\dll\OpenAL32.dll"
  File "..\mingwlibs\dll\wrap_oal.dll"
  File "..\mingwlibs\dll\vorbisfile.dll"
  File "..\mingwlibs\dll\vorbis.dll"
  File "..\mingwlibs\dll\ogg.dll"

  ; Use SDL 1.2.10 because SDL 1.2.{9,11,12} break keyboard layout.
  File "..\external\SDL.dll"

  ; Old DLLs, not needed anymore
  ; (python upgraded to 25, MSVC*71.dll was only needed by MSVC compiled unitsync.dll)
  Delete "$INSTDIR\python24.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\MSVCR71.dll"

  File "..\external\SelectionEditor.exe"
  Delete "$INSTDIR\settingstemplate.xml"

;New Settings Program

  File "downloads\springsettings.exe"

; Dlls neede for it
  File /r "..\installer\Springlobby\SettingsDlls\*.dll"

  ; DLLs
  File "..\external\MSVCR71.dll"
  File "..\external\MSVCP71.dll"
  File "..\mingwlibs\dll\DevIL.dll"
  File "..\mingwlibs\dll\freetype6.dll"
  File "..\mingwlibs\dll\ILU.dll"
  File "..\external\zlibwapi.dll"

  File "..\game\PALETTE.PAL"

${IfNot} ${FileExists} "$INSTDIR\selectkeys.txt"
  File "..\game\selectkeys.txt"
${EndIf}

${IfNot} ${FileExists} "$INSTDIR\uikeys.txt"
  File "..\game\uikeys.txt"
${EndIf}

${IfNot} ${FileExists} "$INSTDIR\cmdcolors.txt"
  File "..\game\cmdcolors.txt"
${EndIf}

${IfNot} ${FileExists} "$INSTDIR\ctrlpanel.txt"
  File "..\game\ctrlpanel.txt"
${EndIf}

${IfNot} ${FileExists} "$INSTDIR\teamcolors.lua"
  File "..\game\teamcolors.lua"
${EndIf}

  SetOutPath "$INSTDIR\fonts"
  File "..\game\fonts\FreeSansBold.otf"

  ; Remove Luxi.ttf, it has been replaced by FreeSansBold
  Delete "$INSTDIR\Luxi.ttf"
  Delete "$INSTDIR\fonts\Luxi.ttf"

  ; AI Interfaces
!macro InstallAIInterface aiIntName
!ifdef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
  SetOutPath "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
  File /r /x *.a /x *.def /x *.7z /x *.dbg "..\game\AI\Interfaces\${aiIntName}\${AI_INT_VERS}\*.*"
  ;buildbot creates 7z, and those get included in installer, fix here until buildserv got fixed
  ;File /r "..\AI\Interfaces\${aiIntName}\data\*.*"
  !undef AI_INT_VERS
!endif
!macroend
  !insertmacro InstallAIInterface "C"
  !insertmacro InstallAIInterface "Java"

  ; Skirmish AIs -> each Skirmish AI has its own .nsh file
!macro InstallSkirmishAI skirAiName
!ifdef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
  SetOutPath "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
  File /r /x *.a /x *.def /x *.7z /x *.dbg "..\game\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}\*.*"
  ;File /r "..\AI\Skirmish\${skirAiName}\data\*.*"
  !undef SKIRM_AI_VERS
!endif
!macroend
  ;TODO: Fix the vc projects to use the same names.
  !insertmacro InstallSkirmishAI "NullAI"
  !insertmacro InstallSkirmishAI "NullOOJavaAI"

; Default content
  SetOverWrite on
  SetOutPath "$INSTDIR\base"

  File "..\game\base\springcontent.sdz"
  File "..\game\base\maphelper.sdz"
  File "..\game\base\cursors.sdz"
  SetOutPath "$INSTDIR\base\spring"
  File "..\game\base\spring\bitmaps.sdz"

;!ifndef SP_UPDATE
${IfNot} ${FileExists} "$INSTDIR\spring.exe"
  ; Demofile file association
  !insertmacro APP_ASSOCIATE "sdf" "spring.demofile" "Spring demo file" "$INSTDIR\spring.exe,0" "Open with Spring" "$\"$INSTDIR\spring.exe$\" $\"%1$\""
  !insertmacro UPDATEFILEASSOC
${EndIf}
;!endif ; SP_UPDATE

!else

  ; Main files
  Delete "$INSTDIR\spring.exe"
  Delete "$INSTDIR\spring.def"
  Delete "$INSTDIR\unitsync.dll"
  Delete "$INSTDIR\TASServer.jar"
  Delete "$INSTDIR\PALETTE.PAL"
  Delete "$INSTDIR\SelectionEditor.exe"
  Delete "$INSTDIR\selectkeys.txt"
  Delete "$INSTDIR\uikeys.txt"
  Delete "$INSTDIR\cmdcolors.txt"
  Delete "$INSTDIR\ctrlpanel.txt"
  Delete "$INSTDIR\teamcolors.lua"

  ;New Settings Program
  Delete "$INSTDIR\springsettings.exe"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\wxbase28u_gcc_custom.dll"
  Delete "$INSTDIR\wxbase28u_net_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_adv_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_core_gcc_custom.dll"
  Delete "$INSTDIR\springsettings.conf"

  ; DLLs
  Delete "$INSTDIR\DevIL.dll"
  Delete "$INSTDIR\freetype6.dll"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\ILU.dll"
  Delete "$INSTDIR\ILUT.dll"
  Delete "$INSTDIR\python25.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\zlibwapi.dll"
  Delete "$INSTDIR\MSVCR71.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\OpenAL32.dll"
  Delete "$INSTDIR\wrap_oal.dll"
  Delete "$INSTDIR\vorbisfile.dll"
  Delete "$INSTDIR\vorbis.dll"
  Delete "$INSTDIR\ogg.dll"


  Delete "$INSTDIR\PALETTE.PAL"

  ; Fonts
  Delete "$INSTDIR\fonts\FreeSansBold.otf"
  RmDir "$INSTDIR\fonts"

  ; Skirmish AIs -> each Skirmish AI has its own .nsh file
!macro DeleteSkirmishAI skirAiName
!ifndef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
  RmDir /r "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
  !undef SKIRM_AI_VERS
!endif
!macroend
  !insertmacro DeleteSkirmishAI "NullAI"
  !insertmacro DeleteSkirmishAI "NullOOJavaAI"
  RmDir "$INSTDIR\AI\Global"
  RmDir "$INSTDIR\AI\Skirmish"

  RmDir "$INSTDIR\AI\Helper-libs"
  RmDir /r "$INSTDIR\AI"

  ; AI Interfaces
!macro DeleteAIInterface aiIntName
!ifndef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
  RmDir /r "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
  !undef AI_INT_VERS
!endif
!macroend
  !insertmacro DeleteAIInterface "C"
  !insertmacro DeleteAIInterface "Java"

  ; base content
  Delete "$INSTDIR\base\spring\bitmaps.sdz"
  Delete "$INSTDIR\base\springcontent.sdz"
  Delete "$INSTDIR\base\maphelper.sdz"
  Delete "$INSTDIR\base\cursors.sdz"
  RmDir "$INSTDIR\base\spring"
  RmDir "$INSTDIR\base"

  ; Generated stuff from the installer
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  ; Generated stuff from running spring
  Delete "$INSTDIR\ArchiveCacheV7.lua"
  Delete "$INSTDIR\unitsync.log"
  Delete "$INSTDIR\infolog.txt"
  Delete "$INSTDIR\ext.txt"
  RmDir "$INSTDIR\demos"

  ; Demofile file association
  !insertmacro APP_UNASSOCIATE "sdf" "spring.demofile"
  
  MessageBox MB_YESNO|MB_ICONQUESTION "Do you want me to completely remove all spring related files?$\n\
  All maps, mods, screenshots and your settings will be removed. $\n\
  CAREFULL: ALL CONTENTS OF YOUR SPRING INSTALLATION DIRECTORY WILL BE REMOVED!" \
  IDNO skip_purge
  
  RmDir /r "$INSTDIR"
  Delete "$LOCALAPPDATA\springsettings.cfg"
  Delete "$APPDATA\springlobby.conf"

skip_purge:

!endif
