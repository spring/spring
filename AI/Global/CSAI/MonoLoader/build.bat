rem you'll need:
rem - mingw
rem - Mono 1.1.18 (1.2 will NOT work, at least not on Windows)
rem - TASpring
rem - TASpring sourcecode (taspring website )

rem modify the following four paths for your environment:
set MINGWDIR=g:\bin\mingw
set MONODIR=g:\bin\Mono-1.1.18
set SPRINGSOURCE=..\..\..\..
set SPRINGAPPLICATION=..\..\..\..\game

set PATH=%PATH%;%MINGWDIR%\bin;%MONODIR%\bin

copy /y %SPRINGSOURCE%\rts\ExternalAI\GlobalAIInterfaces\GlobalAIInterfaces.dll .
call gmcs -debug -reference:GlobalAIInterfaces.dll MonoABICInterfaceGenerator.cs
call mono --debug MonoABICInterfaceGenerator.exe

call gmcs -debug -reference:GlobalAIInterfaces.dll MonoAbicWrappersGenerator.cs
call mono --debug MonoAbicWrappersGenerator.exe

set WRAPPERS=AbicIAICallbackWrapper_generated.cs AbicIFeatureDefWrapper_generated.cs AbicIMoveDataWrapper_generated.cs AbicIUnitDefWrapper_generated.cs
call gmcs -debug -target:library -reference:GlobalAIInterfaces.dll -out:CSAIMono.dll CSAICInterface.cs ABICInterface_generated.cs %WRAPPERS%
copy /y CSAIMono.dll %SPRINGAPPLICATION%
copy /y CSAIMono.dll.mdb %SPRINGAPPLICATION%

set CCOPTIONS=-I"%SPRINGSOURCE%\rts\System" -I"%SPRINGSOURCE%\rts"
set CCOPTIONS=%CCOPTIONS% -I"%MONODIR%\include" -I"%MONODIR%\lib\glib-2.0\include" -I"%MONODIR%\include\glib-2.0"

set LINKOPTIONS= -L "%MONODIR%/lib"  -lglib-2.0.dll -lgmodule-2.0.dll -lgthread-2.0.dll -lgobject-2.0.dll -lmono.dll

g++ -DBUILDING_AI -DSTREFLOP_X87=1 %CCOPTIONS% -c CSAILoaderMono.cpp
dllwrap --driver-name g++ -o csailoadermono.dll CSAILoaderMono.o %SPRINGAPPLICATION%\Spring.a %LINKOPTIONS%

copy /y CSAILoaderMono.dll "%SPRINGAPPLICATION%\AI\Bot-libs"
rem copy /y CSAILoaderMono.dll "%SPRINGAPPLICATION%"


