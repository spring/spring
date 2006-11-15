rem you'll need:
rem - mingw
rem - TASpring
rem - TASpring sourcecode (taspring website )

rem modify the following four paths for your environment:
set MINGWDIR=h:\bin\mingw
set SPRINGSOURCE=..\..\..
set SPRINGAPPLICATION=..\..\..\game

set PATH=%PATH%;%MINGWDIR%\bin

set GCCOPTIONS=-I"%SPRINGSOURCE%\rts\System" -I"%SPRINGSOURCE%\rts"
set LINKOPTIONS=

rem dllwrap %LINKOPTIONS% --def %SPRINGSOURCE%\rts\ExternalAI\GlobalAICInterface\def_generated.def -o myaimingw.dll myaimingw.o %SPRINGAPPLICATION%\Spring.lib
rem rem dllwrap %LINKOPTIONS% -o myaimingw.dll myaimingw.o H:\bin\games\taspring\Application\TASpring\AI\Bot-libs\myaimingwloader.dll


gcc -DBUILDING_AI %GCCOPTIONS% -c myaimingw.cpp
dllwrap --driver-name g++ -o myaimingw.dll myaimingw.o %SPRINGAPPLICATION%\Spring.a

rem link /lib /machine:i386 /def:%SPRINGAPPLICATION%\Spring.def /OUT:spring.lib /NAME:spring.exe
rem dllwrap %LINKOPTIONS% -o myaimingw.dll myaimingw.o %SPRINGAPPLICATION%\Spring.lib
rem link /lib /machine:i386 /def:%SPRINGAPPLICATION%\Spring.def /OUT:spring.lib /NAME:spring.exe
rem ld --warn-unresolved-symbols --verbose --traditional-format -o myaimingw.dll myaimingw.o %SPRINGAPPLICATION%\Spring.lib

copy /y myaimingw.dll "%SPRINGAPPLICATION%\AI\Bot-libs"


