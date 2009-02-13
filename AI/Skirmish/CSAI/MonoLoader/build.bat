rem you'll need:
rem - mingw
rem - Mono 1.1.18 (or maybe 1.2)
rem - TASpring
rem - TASpring sourcecode (taspring website )
rem - the appropriate dlls in your taspring directory

rem modify the following four paths for your environment:
set MONODIR=C:\Mono-1.9.1
set SPRINGSOURCE=..\..\..\..
set SPRINGAPPLICATION=..\..\..\..\game


set PATH=%PATH%;%MINGWDIR%\bin;%MONODIR%\bin

copy /y ..\CSAIInterfaces\CSAIInterfaces.dll .
call gmcs -debug -reference:CSAIInterfaces.dll MonoABICInterfaceGenerator.cs
call mono --debug MonoABICInterfaceGenerator.exe

call gmcs -debug -reference:CSAIInterfaces.dll MonoAbicWrappersGenerator.cs
call mono --debug MonoAbicWrappersGenerator.exe

call gmcs -debug generatebindingspaste.cs
call mono --debug generatebindingspaste.exe

rem MonoLoaderProxy is lagtastic, so dont use
rem call gmcs -debug -target:library -reference:CSAIInterfaces.dll MonoLoaderProxy.cs
rem copy /y MonoLoaderProxy.dll %SPRINGAPPLICATION%
rem copy /y MonoLoaderProxy.dll.mdb %SPRINGAPPLICATION%

set WRAPPERS=AbicIAICallbackWrapper_generated.cs AbicIFeatureDefWrapper_generated.cs AbicIMoveDataWrapper_generated.cs AbicIUnitDefWrapper_generated.cs
call gmcs -debug -target:library -reference:CSAIInterfaces.dll -out:CSAIMono.dll CSAICInterface.cs ABICInterface_generated.cs %WRAPPERS%
copy /y CSAIMono.dll %SPRINGAPPLICATION%
copy /y CSAIMono.dll.mdb %SPRINGAPPLICATION%
copy /y CSAIMono.xml %SPRINGAPPLICATION%

set CCOPTIONS=-I"%SPRINGSOURCE%\rts\System" -I"%SPRINGSOURCE%\rts"
set CCOPTIONS=%CCOPTIONS% -I"..\ABICompatibilityLayer"
set CCOPTIONS=%CCOPTIONS% -I"%MONODIR%\include" -I"%MONODIR%\include\mono-1.0" -I"%MONODIR%\lib\glib-2.0\include" -I"%MONODIR%\include\glib-2.0"

set LINKOPTIONS= -L "%MONODIR%/lib"  -lglib-2.0.dll -lgmodule-2.0.dll -lgthread-2.0.dll -lgobject-2.0.dll -lmono.dll

g++ -DBUILDING_SKIRMISH_AI -DSTREFLOP_X87=1 %CCOPTIONS% -c CSAILoaderMono.cpp
dllwrap --driver-name g++ -o csailoadermono.dll CSAILoaderMono.o %LINKOPTIONS%

copy /y CSAILoaderMono.dll "%SPRINGAPPLICATION%\AI\Skirmish\impls"


