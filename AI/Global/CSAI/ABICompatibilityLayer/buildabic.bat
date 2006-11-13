@echo off
if .%2==. goto :usage
goto :run

:usage
echo Usage: buildabic [abic dll name] [ai dll name]
goto :eof

:run

set ABICDLLNAME=%1
set AIDLLNAME=%2

rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )

rem modify the following five paths for your environment:
set VISUALCPPDIRECTORY=h:\bin\microsoft visual C++ toolkit 2003
set PLATFORMSDK=h:\bin\microsoft platform sdk for windows xp sp2
set SPRINGSOURCE=h:\bin\games\taspring\taspring_0.73b1_src\taspring_0.73b1
set SPRINGAPPLICATION=h:\bin\games\taspring\application\taspring
set FRAMEWORKDIRECTORY=c:\windows\microsoft.net\framework\v2.0.50727

set PATH=%PATH%;%VISUALCPPDIRECTORY%\bin;%FRAMEWORKDIRECTORY%

set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include" /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"

set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

rem generate generated files
copy /y ..\CSAIInterfaces\CSAIInterfaces.dll .
csc /debug ABICCodeGenerator.cs /reference:CSAIInterfaces.dll
ABICCodeGenerator

rem define BUILDING_ABIC to setup dll exports correctly (see dllbuild.h)
cl /MD /D BUILDING_ABIC /c AbicAICallback.cpp
cl /MD /D BUILDING_ABIC /c AbicLoader.cpp

cl /MD /D BUILDING_ABIC /c Platform\SharedLib.cpp
cl /MD /D BUILDING_ABIC /c Platform\Win\DllLib.cpp

set OBJECTS=AbicLoader.obj AbicProxy.obj AbicAICallback.obj SharedLib.obj DllLib.obj

rem AIDLLNAME defines the dll that the ABIC dll will try to load at runtime
cl /MD /D BUILDING_ABIC /DAIDLLNAME=%AIDLLNAME% /c AbicProxy.cpp
link /dll /out:%ABICDLLNAME% %OBJECTS%

copy /y CSAIInterfaces.dll %SPRINGAPPLICATION%
copy /y %ABICDLLNAME% "%SPRINGAPPLICATION%\AI\Bot-libs"
