rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )
rem - Framework.Net

rem modify the following paths for your environment:
rem set VISUALCPPDIRECTORY=j:\bin\microsoft visual C++ toolkit 2003
rem set PLATFORMSDK=j:\bin\microsoft platform sdk for windows xp sp2
set SPRINGSOURCE=J:\taspring\spring_0.74b3_src\spring_0.74b3
set SPRINGAPPLICATION=j:\taspring\release
set FRAMEWORKDOTNET=%windir%\microsoft.net\framework\v2.0.50727
set MINGWDIR=j:\devtools\dev-cpp

set PATH=%PATH%;%MINGWDIR%\bin;%FRAMEWORKDOTNET%

rem set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" 
rem set CL=%CL% /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include"
rem set CL=%CL% /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"
set CL=%CL% /I..\AbicWrappers
rem set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

rem cd ..\AbicWrappers
rem call generate
rem cd ..\CSAILoader

copy /y ..\CSAIInterfaces\CSAIInterfaces.dll .
rem csc /debug AbicWrapperGenerator.cs /reference:CSAIInterfaces.dll
rem AbicWrapperGenerator.exe

csc /debug CSAIProxyGenerator.cs /reference:CSAIInterfaces.dll
CSAIProxyGenerator.exe

cl /clr /MD /c CSAILoader.cpp /FUCSAIInterfaces.dll
cl /clr /MD /c CSAIProxy.cpp /FUCSAIInterfaces.dll

link /lib /machine:i386 /def:%SPRINGAPPLICATION%\Spring.def /OUT:spring.lib /NAME:spring.exe
link /dll /NOENTRY msvcrt.lib /NODEFAULTLIB:nochkclr.obj csailoader.obj CSAIProxy.obj spring.lib /INCLUDE:__DllMainCRTStartup@12

copy /y csailoader.dll "%SPRINGAPPLICATION%"
copy /y csailoader.xml "%SPRINGAPPLICATION%"
