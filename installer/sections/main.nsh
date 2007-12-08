
!ifdef INSTALL
  SetOutPath "$INSTDIR"
  SetOverWrite on

  ; Main stuff
  File "..\game\spring.exe"
  File "..\game\spring.def"

  ; DLLs (updated in mingwlibs-v8)
  File "..\mingwlibs\dll\glew32.dll"
  File "..\mingwlibs\dll\python25.dll"
  File "..\mingwlibs\dll\SDL.dll"
  File "..\mingwlibs\dll\zlib1.dll"
  
  ; Old DLLs, not needed anymore
  ; (python upgraded to 25, MSVC*71.dll was only needed by MSVC compiled unitsync.dll)
  Delete "$INSTDIR\python24.dll"
  Delete "$INSTDIR\MSVCP71.dll"
  Delete "$INSTDIR\MSVCR71.dll"
  
!ifndef SP_UPDATE
  File "..\game\settings.exe"
  File "..\game\selectkeys.txt"
  File "..\game\uikeys.txt"
  File "..\game\cmdcolors.txt"
  File "..\game\ctrlpanel.txt"

  File "..\game\SelectionEditor.exe"
  ;File "..\game\settingstemplate.xml"

  ; DLLs
  File "..\game\MSVCR71.dll"
  File "..\game\MSVCP71.dll"
  File "..\mingwlibs\dll\DevIL.dll"
  File "..\mingwlibs\dll\freetype6.dll"
  File "..\mingwlibs\dll\ILU.dll"
  File "..\game\zlibwapi.dll"

  File "..\game\PALETTE.PAL"

!endif ; SP_UPDATE

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

; Default content
  SetOverWrite on
  SetOutPath "$INSTDIR\base"

  File "..\game\base\springcontent.sdz"
  SetOutPath "$INSTDIR\base\spring"
  File "..\game\base\spring\bitmaps.sdz"

!ifndef SP_UPDATE

  ; Demofile file association
  !insertmacro APP_ASSOCIATE "sdf" "spring.demofile" "Spring demo file" "$INSTDIR\spring.exe,0" "Open with Spring" "$\"$INSTDIR\spring.exe$\" $\"%1$\""
  !insertmacro UPDATEFILEASSOC 

!endif ; SP_UPDATE

!else

  ; Main files
  Delete "$INSTDIR\spring.exe"
  Delete "$INSTDIR\spring.def"
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
  Delete "$INSTDIR\python25.dll"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\zlibwapi.dll"

  Delete "$INSTDIR\PALETTE.PAL"

  ; Fonts
  Delete "$INSTDIR\fonts\Luxi.ttf"
  RmDir "$INSTDIR\fonts"
  
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

  Delete "$INSTDIR\base\spring\bitmaps.sdz"
  Delete "$INSTDIR\base\springcontent.sdz"
  RmDir "$INSTDIR\base\spring"
  RmDir "$INSTDIR\base"

  ; Generated stuff from the installer
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  ; Generated stuff from running spring
  Delete "$INSTDIR\ArchiveCacheV4.txt"
  Delete "$INSTDIR\ArchiveCacheV5.txt"
  Delete "$INSTDIR\infolog.txt"
  Delete "$INSTDIR\ext.txt"
  Delete "$INSTDIR\demos\test.sdf"
  RmDir "$INSTDIR\demos"

  ; Demofile file association
  !insertmacro APP_UNASSOCIATE "sdf" "spring.demofile"

!endif
