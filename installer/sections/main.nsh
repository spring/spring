
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  SetOverWrite on

  ; Main stuff
  File "..\game\spring.exe"
  File "..\game\spring.def"
  File "..\game\unitsync.dll"
  CreateDirectory "$INSTDIR\maps"
  CreateDirectory "$INSTDIR\mods"

  ; DLLs (updated in mingwlibs-v8)
  File "..\mingwlibs\dll\glew32.dll"
  File "..\mingwlibs\dll\python25.dll"
  File "..\mingwlibs\dll\zlib1.dll"

  ; Use SDL 1.2.10 because SDL 1.2.{9,11,12} break keyboard layout.
  File "..\external\SDL.dll"

  ; Old DLLs, not needed anymore
  ; (python upgraded to 25, MSVC*71.dll was only needed by MSVC compiled unitsync.dll)
  Delete "$INSTDIR\python24.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\MSVCR71.dll"

  ; Delete Previous settings.exe
  Delete "..\game\settings.exe"
  File "..\external\SelectionEditor.exe"
  Delete "$INSTDIR\settingstemplate.xml"

;New Settings Program

  inetc::get \
  "http://www.springlobby.info/installer/springsettings.exe" "$INSTDIR\springsettings.exe"

  File "..\external\mingwm10.dll"
  File "..\external\wxbase28u_gcc_custom.dll"
  File "..\external\wxbase28u_net_gcc_custom.dll"
  File "..\external\wxmsw28u_adv_gcc_custom.dll"
  File "..\external\wxmsw28u_core_gcc_custom.dll"

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

;!endif ; SP_UPDATE

  SetOutPath "$INSTDIR\startscripts"
  File "..\game\startscripts\*.lua"

  ; Remove shaders, they are now in springcontent.sdz
  Delete "$INSTDIR\shaders\*.fp"
  Delete "$INSTDIR\shaders\*.vp"
  Delete "$INSTDIR\shaders\*.glsl"
  RmDir "$INSTDIR\shaders"

  SetOutPath "$INSTDIR\fonts"
  File "..\game\fonts\Luxi.ttf"

  ; Remove Luxi.ttf, it has been moved to fonts/Luxi.ttf
  Delete "$INSTDIR\Luxi.ttf"

  ; AI Interfaces
!macro InstallAIInterface aiIntName
!ifdef INSTALL_AIS
  SetOutPath "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS_${aiIntName}}"
  File /x *.a /x *.def /x *.dbg "${BUILDDIR}\AI\Interfaces\${aiIntName}\${AI_INT_VERS_${aiIntName}}\*.*"
  File /r /x .svn "..\AI\Interfaces\${aiIntName}\data\*.*"
!endif
!macroend
  !insertmacro InstallAIInterface "C"
  !insertmacro InstallAIInterface "Java"

;  ; Group AIs -> dont exist anymore
;  ;TODO: Fix the vc projects to use the same names.
;  SetOutPath "$INSTDIR\AI\Helper-libs"
;  File "..\game\AI\Helper-libs\CentralBuildAI.dll"
;  File "..\game\AI\Helper-libs\MetalMakerAI.dll"
;  File "..\game\AI\Helper-libs\SimpleFormationAI.dll"
;  File "..\game\AI\Helper-libs\RadarAI.dll"
;  File "..\game\AI\Helper-libs\MexUpgraderAI.dll"
;  File "..\game\AI\Helper-libs\EconomyAI.dll"
;  File "..\game\AI\Helper-libs\ReportIdleAI.dll"

  ; Skirmish AIs -> each Skirmish AI has its own .nsh file
!macro InstallSkirmishAI skirAiName
!ifdef INSTALL_AIS
  SetOutPath "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIR_AI_VERS_${skirAiName}}"
  File /x *.a /x *.def /x *.dbg "${BUILDDIR}\AI\Skirmish\${skirAiName}\${SKIR_AI_VERS_${skirAiName}}\*.*"
  File /r /x .svn "..\AI\Skirmish\${skirAiName}\data\*.*"
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
  Delete "$INSTDIR\PALETTE.PAL"
  Delete "$INSTDIR\SelectionEditor.exe"
  Delete "$INSTDIR\selectkeys.txt"
  Delete "$INSTDIR\uikeys.txt"
  Delete "$INSTDIR\cmdcolors.txt"
  Delete "$INSTDIR\ctrlpanel.txt"
  Delete "$INSTDIR\teamcolors.lua"
  ;Delete "$INSTDIR\settings.exe"

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


  Delete "$INSTDIR\PALETTE.PAL"

  ; Fonts
  Delete "$INSTDIR\fonts\Luxi.ttf"
  RmDir "$INSTDIR\fonts"

  ; Skirmish AIs -> each Skirmish AI has its own .nsh file
!macro DeleteSkirmishAI skirAiName
!ifdef INSTALL_AIS
  RmDir /r "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIR_AI_VERS_${skirAiName}}"
!endif
!macroend
  !insertmacro DeleteSkirmishAI "NullAI"
  !insertmacro DeleteSkirmishAI "NullOOJavaAI"
  RmDir "$INSTDIR\AI\Global"
  RmDir "$INSTDIR\AI\Skirmish"

;  ; Group AIs -> dont exist anymore
;  ; AI Helper dlls
;  Delete "$INSTDIR\AI\Helper-libs\CentralBuildAI.dll"
;  Delete "$INSTDIR\AI\Helper-libs\MetalMakerAI.dll"
;  Delete "$INSTDIR\AI\Helper-libs\SimpleFormationAI.dll"
;  Delete "$INSTDIR\AI\Helper-libs\RadarAI.dll"
;  Delete "$INSTDIR\AI\Helper-libs\MexUpgraderAI.dll"
;  Delete "$INSTDIR\AI\Helper-libs\EconomyAI.dll"
;  Delete "$INSTDIR\AI\Helper-libs\ReportIdleAI.dll"
  RmDir "$INSTDIR\AI\Helper-libs"
  RmDir "$INSTDIR\AI"

  ; AI Interfaces
!macro DeleteAIInterface aiIntName
!ifdef INSTALL_AIS
  RmDir /r "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS_${aiIntName}"
!endif
!macroend
  !insertmacro DeleteAIInterface "C"
  !insertmacro DeleteAIInterface "Java"

  ; Startscript
  Delete "$INSTDIR\startscripts\aistartscripttest.lua"
  Delete "$INSTDIR\startscripts\cmdrscript.lua"
  Delete "$INSTDIR\startscripts\missionhelper.lua"
  Delete "$INSTDIR\startscripts\missiontest.lua"
  Delete "$INSTDIR\startscripts\ordertroops.lua"
  Delete "$INSTDIR\startscripts\testscript.lua"
  RmDir "$INSTDIR\startscripts"

  ; base content
  Delete "$INSTDIR\base\spring\bitmaps.sdz"
  Delete "$INSTDIR\base\springcontent.sdz"
  Delete "$INSTDIR\base\maphelper.sdz"
  Delete "$INSTDIR\base\cursors.sdz"
  RmDir "$INSTDIR\base\spring"
  RmDir "$INSTDIR\base"

  ; XTA from previous installer
  Delete "$INSTDIR\mods\XTAPE.sdz"

  ; Generated stuff from the installer
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  ; Generated stuff from running spring
  Delete "$INSTDIR\ArchiveCacheV4.txt"
  Delete "$INSTDIR\ArchiveCacheV5.txt"
  Delete "$INSTDIR\ArchiveCacheV6.txt"
  Delete "$INSTDIR\ArchiveCacheV7.lua"
  Delete "$INSTDIR\unitsync.log"
  Delete "$INSTDIR\infolog.txt"
  Delete "$INSTDIR\ext.txt"
  Delete "$INSTDIR\demos\test.sdf"
  RmDir "$INSTDIR\demos"

  ; Demofile file association
  !insertmacro APP_UNASSOCIATE "sdf" "spring.demofile"

!endif
