rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )

rem modify the following four paths for your environment:
set VISUALCPPDIRECTORY=h:\bin\microsoft visual C++ toolkit 2003
set PLATFORMSDK=h:\bin\microsoft platform sdk for windows xp sp2
set SPRINGSOURCE=..\..\..
set SPRINGAPPLICATION=..\..\..\game

set PATH=%PATH%;%VISUALCPPDIRECTORY%\bin

set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include" /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"

set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

rem cl /MD /D BUILDING_AI /c myai.cpp
rem link /dll myai.obj %SPRINGAPPLICATION%\Spring.lib

link /lib /machine:i386 /def:%SPRINGAPPLICATION%\Spring.def /OUT:spring.lib /NAME:spring.exe
cl /LD /D BUILDING_AI myai.cpp spring.lib

copy /y myai.dll "%SPRINGAPPLICATION%\AI\Bot-libs"


