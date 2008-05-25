rem if .%2==. goto :usage
goto :run

:usage
echo Usage: buildabic [abic dll name] [ai dll name]
goto :eof

:run

set ABICDLLNAME=%1
set AIDLLNAME=%2

if .%ABICDLLNAME%==. set ABICDLLNAME=csaibic.dll
if .%AIDLLNAME%==. set AIDLLNAME=csailoader.dll

rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )

rem modify the following paths for your environment:
set SPRINGSOURCE=%~dp0../../../..
set SPRINGAPPLICATION=%~dp0../../../../game
set FRAMEWORKDIRECTORY=%windir%\microsoft.net\framework\v2.0.50727

set PATH=%PATH%;%MINGDIR%\bin;%FRAMEWORKDIRECTORY%

rem set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include" /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"

rem set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

rem generate generated files
copy /y ..\CSAIInterfaces\CSAIInterfaces.dll .
copy /y ..\CSAIInterfaces\CSAIInterfaces.pdb .
csc /debug GlobalAICInterfaceGenerator.cs /reference:CSAIInterfaces.dll
globalaicinterfacegenerator

set CCOPTIONS=-I"%SPRINGSOURCE%\rts\System" -I"%SPRINGSOURCE%\rts"

rem define BUILDING_ABIC to setup dll exports correctly (see dllbuild.h)
gcc -c %CCOPTIONS% -DBUILDING_ABIC AbicAICallback.cpp
gcc -c %CCOPTIONS% -DBUILDING_ABIC AbicLoader.cpp

gcc -c %CCOPTIONS% -DBUILDING_ABIC Platform\SharedLib.cpp
gcc -c %CCOPTIONS% -DBUILDING_ABIC Platform\Win\DllLib.cpp

rem AIDLLNAME defines the dll that the ABIC dll will try to load at runtime
gcc -c %CCOPTIONS% -DBUILDING_ABIC -DAIDLLNAME=%AIDLLNAME% AbicProxy.cpp

set OBJECTS=AbicLoader.o AbicProxy.o AbicAICallback.o SharedLib.o DllLib.o
dllwrap --driver-name g++ --dllname %ABICDLLNAME% %OBJECTS%

copy /y GlobalAIInterfaces.dll %SPRINGAPPLICATION%
copy /y %ABICDLLNAME% "%SPRINGAPPLICATION%\AI\Bot-libs"
