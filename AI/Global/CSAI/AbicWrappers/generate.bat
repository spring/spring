rem Regenerates Abic++ wrapper classes
rem
rem generator needs Microsoft.Net or Mono
rem This script uses Microsoft.Net
rem
rem you'll need:
rem - TASpring sourcecode (taspring website )
rem - Framework.Net

rem modify the following four paths for your environment:
set SPRINGSOURCE=%~dp0..\..\..\..
set SPRINGAPPLICATION=%~dp0..\..\..\..\game
set FRAMEWORKDOTNET=%windir%\microsoft.net\framework\v2.0.50727

set PATH=%PATH%;%FRAMEWORKDOTNET%

copy /y %~dp0..\csaiinterfaces\csaiInterfaces.dll .
csc /debug AbicWrapperGenerator.cs /reference:csaiInterfaces.dll
AbicWrapperGenerator.exe
