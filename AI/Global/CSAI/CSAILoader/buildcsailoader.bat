rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )
rem - Framework.Net

rem modify the following four paths for your environment:
set VISUALCPPDIRECTORY=h:\bin\microsoft visual C++ toolkit 2003
set PLATFORMSDK=h:\bin\microsoft platform sdk for windows xp sp2
set SPRINGSOURCE=..\..\..\..
set SPRINGAPPLICATION=..\..\..\..\game
set FRAMEWORKDOTNET=c:\windows\microsoft.net\framework\v2.0.50727

set PATH=%PATH%;%VISUALCPPDIRECTORY%\bin;%FRAMEWORKDOTNET%

set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" 
set CL=%CL% /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include"
set CL=%CL% /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"
set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

copy /y %SPRINGSOURCE%\rts\ExternalAI\GlobalAIInterfaces\GlobalAIInterfaces.dll .
csc /debug AbicWrapperGenerator.cs /reference:GlobalAIInterfaces.dll
AbicWrapperGenerator.exe

csc /debug CSAIProxyGenerator.cs /reference:GlobalAIInterfaces.dll
CSAIProxyGenerator.exe

cl /clr /MD /c CSAILoader.cpp /FUGlobalAIInterfaces.dll
cl /clr /MD /c CSAIProxy.cpp /FUGlobalAIInterfaces.dll

link /lib /machine:i386 /def:%SPRINGAPPLICATION%\Spring.def /OUT:spring.lib /NAME:spring.exe
link /dll /NOENTRY msvcrt.lib /NODEFAULTLIB:nochkclr.obj csailoader.obj CSAIProxy.obj spring.lib /INCLUDE:__DllMainCRTStartup@12

copy /y csailoader.dll "%SPRINGAPPLICATION%\AI\Bot-libs"
copy /y csailoader.xml "%SPRINGAPPLICATION%\AI\Bot-libs"
