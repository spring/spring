rem This generates the files XXX_generated.*
rem When would you want to run this?
rem - you've modified the interface definition in ..\GlobalAIInterface
rem
rem Do I need to use this?
rem - only if you modified ..\GlobalAIInterface
rem
rem Do I need to use this to add new functions?
rem - you can add new fucntions directly to AbicAICallback.cpp and AbicAICallback.h
rem
rem Why would I add the function via ..\GlobalAIInterface?
rem - GlobalAIInterface is trivial to parse using Mono, so is good for writing generators, binding with
rem   other languages etc.
rem
rem
rem You need either Mono or Framework.Net to run the generator
rem This file works with Framework.Net

set FRAMEWORKDOTNET=c:\windows\microsoft.net\framework\v2.0.50727
set SPRINGSOURCE=..\..\..

set PATH=%PATH%;%FRAMEWORKDOTNET%

copy /y %SPRINGSOURCE%\rts\ExternalAI\GlobalAIInterfaces\GlobalAIInterfaces.dll .
csc /debug GlobalAICInterfaceGenerator.cs /reference:GlobalAIInterfaces.dll
GlobalAICInterfaceGenerator.exe

