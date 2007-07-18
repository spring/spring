
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  SetOverWrite on

  ; Main stuff
  File "..\game\spring.exe"
  File "..\game\spring.def"

  ; Devil.dll has been upgraded to 1.6.8rc2
  ; (even though the DLL properties say 1.6.5, this is a Devil bug...)
  File "..\mingwlibs\dll\DevIL.dll"

  ; SDL is downgraded
  ; grab SDL 1.2.10 from game directory, the one in mingwlibs (1.2.11) breaks
  ; azerty/qwerty for non-chat input.
  File "..\game\SDL.dll"

  ; ILU.dll and python25.dll are new since 0.75b1
  File "..\mingwlibs\dll\ILU.dll"
  File "..\mingwlibs\dll\python25.dll"

  ; zlibwapi.dll and MSVCP71.dll is removed since 0.75b1
  ; unsafe.... hotfixing them in again
  ; Delete "$INSTDIR\zlibwapi.dll"
  ; Delete "$INSTDIR\MSVCP71.dll"

!ifndef SP_UPDATE
  File "..\game\settings.exe"
  File "..\game\selectkeys.txt"
  File "..\game\uikeys.txt"
  File "..\game\cmdcolors.txt"
  File "..\game\ctrlpanel.txt"

  File "..\game\Luxi.ttf"
  File "..\game\SelectionEditor.exe"
  ;File "..\game\settingstemplate.xml"

  ; DLLs
  File "..\game\MSVCR71.dll"
  File "..\game\MSVCP71.dll"
  File "..\mingwlibs\dll\freetype6.dll"
  File "..\mingwlibs\dll\glew32.dll"
  File "..\mingwlibs\dll\zlib1.dll"
  File "..\game\zlibwapi.dll"

  File "..\game\PALETTE.PAL"
  
  ; Gamedata - stored in springcontent.sdz now. If it's stored on disk then mods can't override it
  Delete "$INSTDIR\gamedata\*.tdf"
  RmDir "$INSTDIR\gamedata"

!endif ; SP_UPDATE

  SetOutPath "$INSTDIR\startscripts"
  File "..\game\startscripts\*.lua"

  SetOutPath "$INSTDIR\shaders"
  File "..\game\shaders\*.fp"
  File "..\game\shaders\*.vp"
  File "..\game\shaders\*.glsl"


; TODO: Fix the vc projects to use the same names.
  SetOutPath "$INSTDIR\AI\Helper-libs"
  File "..\game\AI\Helper-libs\CentralBuildAI.dll"
  File "..\game\AI\Helper-libs\MetalMakerAI.dll"
  File "..\game\AI\Helper-libs\SimpleFormationAI.dll"
  File "..\game\AI\Helper-libs\RadarAI.dll"
  File "..\game\AI\Helper-libs\MexUpgraderAI.dll"
  File "..\game\AI\Helper-libs\EconomyAI.dll"
  File "..\game\AI\Helper-libs\ReportIdleAI.dll"

; TODO: Fix the vc projects to use the same names.
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\TestGlobalAI.dll"

!ifndef SP_UPDATE

; Default content
  SetOverWrite on
  SetOutPath "$INSTDIR\base"

!ifndef NO_TOTALA
  File "..\game\base\tatextures_v062.sdz"
  File "..\game\base\otacontent.sdz"
  File "..\game\base\tacontent_v2.sdz"
!endif

  File "..\game\base\springcontent.sdz"
  SetOutPath "$INSTDIR\base\spring"
  File "..\game\base\spring\bitmaps.sdz"

; Default mod
!ifndef NO_TOTALA
  SetOutPath "$INSTDIR\mods"
  File "..\game\mods\XTAPE.sdz"
!endif

!endif ; SP_UPDATE

  !insertmacro APP_ASSOCIATE_SPECIAL "sdf" "spring.demofile" "Spring demo file" "$INSTDIR\spring.exe,0" "Open with Spring" "$INSTDIR\spring.exe"
  !insertmacro UPDATEFILEASSOC

!else

  ; Main files
  Delete "$INSTDIR\spring.exe"
  Delete "$INSTDIR\spring.def"
  Delete "$INSTDIR\luxi.ttf"
  Delete "$INSTDIR\PALETTE.PAL"
  Delete "$INSTDIR\SelectionEditor.exe"
  Delete "$INSTDIR\selectkeys.txt"
  Delete "$INSTDIR\uikeys.txt"
  Delete "$INSTDIR\cmdcolors.txt"
  Delete "$INSTDIR\ctrlpanel.txt"
  Delete "$INSTDIR\settings.exe"
  Delete "$INSTDIR\settingstemplate.xml"

  ; DLLs
  Delete "$INSTDIR\DevIL.dll"
  Delete "$INSTDIR\freetype6.dll"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\ILU.dll"
  Delete "$INSTDIR\ILUT.dll"
  Delete "$INSTDIR\python24.dll"
  Delete "$INSTDIR\python25.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\MSVCR71.dll"
  Delete "$INSTDIR\zlib1.dll"

  Delete "$INSTDIR\PALETTE.PAL"

  ; Shaders
  Delete "$INSTDIR\shaders\*.fp"
  Delete "$INSTDIR\shaders\*.vp"
  Delete "$INSTDIR\shaders\*.glsl"
  RMDir "$INSTDIR\shaders"
  
  ; AI Bot dlls
  Delete "$INSTDIR\AI\Bot-libs\TestGlobalAI.dll"
  RmDir "$INSTDIR\AI\Bot-libs"

  ; AI Helper dlls
  Delete "$INSTDIR\AI\Helper-libs\CentralBuildAI.dll"
  Delete "$INSTDIR\AI\Helper-libs\MetalMakerAI.dll"
  Delete "$INSTDIR\AI\Helper-libs\SimpleFormationAI.dll"
  Delete "$INSTDIR\AI\Helper-libs\RadarAI.dll"
  Delete "$INSTDIR\AI\Helper-libs\MexUpgraderAI.dll"
  Delete "$INSTDIR\AI\Helper-libs\EconomyAI.dll"
  Delete "$INSTDIR\AI\Helper-libs\ReportIdleAI.dll"
  RmDir "$INSTDIR\AI\Helper-libs"
  RmDir "$INSTDIR\AI"

  ; Startscript
  Delete "$INSTDIR\startscripts\aistartscripttest.lua"
  Delete "$INSTDIR\startscripts\cmdrscript.lua"
  Delete "$INSTDIR\startscripts\missionhelper.lua"
  Delete "$INSTDIR\startscripts\missiontest.lua"
  Delete "$INSTDIR\startscripts\ordertroops.lua"
  Delete "$INSTDIR\startscripts\testscript.lua"
  RmDir "$INSTDIR\startscripts"

  ; base content
!ifndef NO_TOTALA
  Delete "$INSTDIR\base\tatextures_v062.sdz"
  Delete "$INSTDIR\base\tacontent_v2.sdz"
  Delete "$INSTDIR\base\otacontent.sdz"
!endif

  Delete "$INSTDIR\base\spring\bitmaps.sdz"
  Delete "$INSTDIR\base\springcontent.sdz"
  RmDir "$INSTDIR\base\spring"
  RmDir "$INSTDIR\base"

  ; default mod
!ifndef NO_TOTALA
  Delete "$INSTDIR\mods\XTAPE.sdz"
  RmDir "$INSTDIR\mods"
!endif

  ; Generated stuff from the installer
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  ; Generated stuff from running spring
  Delete "$INSTDIR\ArchiveCacheV4.txt"
  Delete "$INSTDIR\infolog.txt"
  Delete "$INSTDIR\ext.txt"
  Delete "$INSTDIR\demos\test.sdf"
  RmDir "$INSTDIR\demos"
!endif
