rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )
rem - Framework.Net

rem Make sure you run this from the Visual STudio 2008 command prompt

rem modify the following paths for your environment:
set PLATFORMSDK=C:\program files\Microsoft SDKs\Windows\v6.1

set SPRINGSOURCE=%~dp0../../../..
set SPRINGAPPLICATION=%~dp0../../../../game
set FRAMEWORKDOTNET=%windir%\microsoft.net\framework\v2.0.50727

set PATH=%PATH%;%MINGWDIR%\bin;%FRAMEWORKDOTNET%

rem set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" 
rem set CL=%CL% /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include"
rem set CL=%CL% /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"
rem set CL=%CL% /I..\AbicWrappers
rem set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

set INCLUDE=%PLATFORMSDK%\include;%INCLUDE%;%SPRINGSOURCE%\rts;%SPRINGSOURCE%\rts\System;%~dp0\..\ABICompatibilityLayer

rem cd ..\AbicWrappers
rem call generate
rem cd ..\CSAILoader

copy /y ..\CSAIInterfaces\CSAIInterfaces.dll .
rem csc /debug AbicWrapperGenerator.cs /reference:CSAIInterfaces.dll
rem AbicWrapperGenerator.exe

csc /debug CSAIProxyGenerator.cs /reference:CSAIInterfaces.dll
CSAIProxyGenerator.exe

cl /clr /MD /c CSAILoader.cpp /DBUILDING_AI /FUCSAIInterfaces.dll
cl /clr /MD /c CSAIProxy.cpp /DBUILDING_AI /FUCSAIInterfaces.dll

link /lib /machine:i386 /def:%SPRINGAPPLICATION%\Spring.def /OUT:spring.lib /NAME:spring.exe
link /dll /NOENTRY msvcrt.lib /NODEFAULTLIB:nochkclr.obj csailoader.obj CSAIProxy.obj spring.lib /INCLUDE:__DllMainCRTStartup@12

copy /y csailoader.dll "%SPRINGAPPLICATION%"
copy /y csailoader.xml "%SPRINGAPPLICATION%"
