@echo This will build all the C++ spring code using visual studio 7...
@echo Make sure you set VSTUDIOPATH to the "c:\vsdotnet\Common7\Tools"
@echo Currently set to: %VSTUDIOPATH%
pause

call %VSTUDIOPATH%\vsvars32
devenv ..\AI\Group\CentralBuildAI\TestAI.sln /build Release
devenv ..\AI\Group\MetalMakerAI\TestAI.sln /build Release
devenv ..\AI\Group\RadarAI\TestAI.sln /build Release
devenv ..\AI\Group\SimpleFormationAI\TestAI.sln /build Release
devenv ..\tools\unitsync\unitsync.sln /build Release
devenv ..\tools\RtsSettings\RtsSettings.sln /build Release
devenv ..\AI\Global\NTAI\NTAI.sln /build Release
devenv ..\AI\Global\TestGlobalAI\TestAI.sln /build Release
devenv ..\rts\build\vstudio7\rts.sln /build "Final release"
