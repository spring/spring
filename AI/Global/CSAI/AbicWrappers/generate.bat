rem Regenerates Abic++ wrapper classes
rem
rem generator needs Microsoft.Net or Mono
rem This script uses Microsoft.Net
rem
rem you'll need:
rem - TASpring sourcecode (taspring website )
rem - Framework.Net

rem modify the following four paths for your environment:
set SPRINGSOURCE=J:\taspring\spring_0.74b3_src\spring_0.74b3
set SPRINGAPPLICATION=j:\taspring\release
set FRAMEWORKDOTNET=%windir%\microsoft.net\framework\v2.0.50727

set PATH=%PATH%;%FRAMEWORKDOTNET%

copy /y %SPRINGSOURCE%\rts\ExternalAI\GlobalAIInterfaces\GlobalAIInterfaces.dll .
csc /debug AbicWrapperGenerator.cs /reference:GlobalAIInterfaces.dll
AbicWrapperGenerator.exe
