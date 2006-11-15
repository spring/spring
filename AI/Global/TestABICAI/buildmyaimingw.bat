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

gcc -DBUILDING_AI %GCCOPTIONS% -c myaimingw.cpp
dllwrap --driver-name g++ -o myaimingw.dll myaimingw.o %SPRINGAPPLICATION%\Spring.a

copy /y myaimingw.dll "%SPRINGAPPLICATION%\AI\Bot-libs"


