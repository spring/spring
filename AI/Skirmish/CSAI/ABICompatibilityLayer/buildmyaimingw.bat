rem you'll need:
rem - mingw
rem - TASpring
rem - TASpring sourcecode (taspring website )

rem modify the following four paths for your environment:
set MINGWDIR=h:\bin\mingw
set SPRINGSOURCE=h:\bin\games\taspring\taspring_0.73b1_src\taspring_0.73b1
set SPRINGAPPLICATION=h:\bin\games\taspring\application\taspring

set PATH=%PATH%;%MINGWDIR%\bin

set GCCOPTIONS=-I"%SPRINGSOURCE%\rts\System" -I"%SPRINGSOURCE%\rts"
rem set LINKOPTIONS=-L%MINGWDIR%/lib -L%MINGWDIR%/lib -lsupc++ -lc -lm -lgcc
rem set LINKOPTIONS=-L%MINGWDIR%/lib -lsupc++ -lm -lgcc
set LINKOPTIONS=--driver-name g++

gcc -DBUILDING_AI %GCCOPTIONS% -c myaimingw.cpp
dllwrap %LINKOPTIONS% -o myaimingw.dll myaimingw.o myaimingwloader.lib

copy /y myaimingw.dll "%SPRINGAPPLICATION%"


