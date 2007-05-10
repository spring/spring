
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  SetOverWrite on

  ; Main stuff
  File "..\game\spring.exe"
  File "..\game\spring.def"

!ifndef SP_UPDATE
  File "..\game\settings.exe"
  File "..\game\selectkeys.txt"
  File "..\game\uikeys.txt"
  File "..\game\cmdcolors.txt"
  File "..\game\ctrlpanel.txt"

  File "..\game\Luxi.ttf"
  File "..\game\SelectionEditor.exe"
  File "..\game\settingstemplate.xml"

  ; DLLs
  ; File "..\game\zlib.dll"
  Delete "$INSTDIR\zlib.dll"
  ; File "..\game\7zxa.dll"
  Delete "$INSTDIR\7zxa.dll"
!ifdef MINGW
  File "..\mingwlibs\dll\*.dll"
!else
  File "..\game\zlibwapi.dll"
  File "..\game\crashrpt.dll"
  File "..\game\dbghelp.dll"
  File "..\game\msvcp71.dll"
  Delete "$INSTDIR\eaxac3.dll"
  Delete "$INSTDIR\freetype6.dll"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\openal32.dll"
  Delete "$INSTDIR\zlib1.dll"
!endif

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

  SetOverWrite on
  ; XTA
  SetOutPath "$INSTDIR\base"

!ifndef NO_TOTALA
  File "..\game\base\tatextures_v062.sdz"
  File "..\game\base\otacontent.sdz"
  File "..\game\base\tacontent_v2.sdz"
!endif

  File "..\game\base\springcontent.sdz"
  SetOutPath "$INSTDIR\base\spring"
  File "..\game\base\spring\bitmaps.sdz"

  SetOutPath "$INSTDIR\mods"

; TODO: use MOD define?
!ifdef NO_TOTALA
  File "..\game\mods\NanoBlobs064.sdz"
!else
  File "..\game\mods\xtape.sd7"
!endif

!endif ; SP_UPDATE

  ; Stuff to always clean up (from old versions etc)
  Delete "$INSTDIR\test.sdf"

  !insertmacro APP_ASSOCIATE_SPECIAL "sdf" "spring.demofile" "Spring demo file" "$INSTDIR\spring.exe,0" "Open with Spring" "$INSTDIR\spring.exe"
  !insertmacro UPDATEFILEASSOC

!else

  ; Main files
  Delete "$INSTDIR\spring.exe"
  Delete "$INSTDIR\spring.def"
  Delete "$INSTDIR\bagge.fnt"
  Delete "$INSTDIR\hpiutil.dll"
  Delete "$INSTDIR\luxi.ttf"
  Delete "$INSTDIR\PALETTE.PAL"
  Delete "$INSTDIR\SelectionEditor.exe"
  Delete "$INSTDIR\selectkeys.txt"
  Delete "$INSTDIR\uikeys.txt"
  Delete "$INSTDIR\cmdcolors.txt"
  Delete "$INSTDIR\ctrlpanel.txt"
  Delete "$INSTDIR\settings.exe"
  Delete "$INSTDIR\settingstemplate.xml"
  Delete "$INSTDIR\zlibwapi.dll"
  Delete "$INSTDIR\crashrpt.dll"
  Delete "$INSTDIR\dbghelp.dll"
  Delete "$INSTDIR\DevIL.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\MSVCR71.dll"

  ; extra DLLs for mingw build
  Delete "$INSTDIR\eaxac3.dll"
  Delete "$INSTDIR\freetype6.dll"
  Delete "$INSTDIR\glew32.dll"
  Delete "$INSTDIR\openal32.dll"
  Delete "$INSTDIR\zlib1.dll"

  Delete "$INSTDIR\tower.sdu"
  Delete "$INSTDIR\palette.pal"

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
  RMDir "$INSTDIR\AI\Helper-libs"
  RMDir "$INSTDIR\AI"

  ; Startscript
  Delete "$INSTDIR\startscripts\aistartscripttest.lua"
  Delete "$INSTDIR\startscripts\testscript.lua"
  Delete "$INSTDIR\startscripts\cmdrscript.lua"
  Delete "$INSTDIR\startscripts\missionhelper.lua"
  Delete "$INSTDIR\startscripts\missiontest.lua"
  RmDir "$INSTDIR\startscripts"

  ; Maps
  Delete "$INSTDIR\maps\paths\SmallDivide.*"
  Delete "$INSTDIR\maps\paths\Mars.*"
  Delete "$INSTDIR\maps\SmallDivide.*"
  Delete "$INSTDIR\maps\Mars.*"
  RmDir "$INSTDIR\maps\paths"
  RmDir "$INSTDIR\maps"

  ; XTA + content
!ifndef NO_TOTALA
  Delete "$INSTDIR\base\tatextures_v062.sdz"
  Delete "$INSTDIR\base\tacontent_v2.sdz"
  Delete "$INSTDIR\base\otacontent.sdz"
!endif

  Delete "$INSTDIR\base\spring\bitmaps.sdz"
  Delete "$INSTDIR\base\springcontent.sdz"
  RmDir "$INSTDIR\base\spring"
  RmDir "$INSTDIR\base"

; TODO: use mods variable?
  Delete "$INSTDIR\mods\xtape.sd7"
  Delete "$INSTDIR\mods\NanoBlobs064.sdz"
  RmDir "$INSTDIR\mods"

  ; Generated stuff from the installer
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  ; Generated stuff from running spring
  Delete "$INSTDIR\ArchiveCacheV4.txt"
  Delete "$INSTDIR\infolog.txt"
  Delete "$INSTDIR\ext.txt"
!endif
