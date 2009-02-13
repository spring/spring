rem you'll need:
rem - Microsoft Visual C++ .Net Express (microsoft.com, free beer )
rem - Microsoft Platform SDK (microsoft.com, free beer )
rem - TASpring sourcecode (taspring website )

rem modify the following four paths for your environment:
set VISUALCPPDIRECTORY=h:\bin\microsoft visual C++ toolkit 2003
set PLATFORMSDK=h:\bin\microsoft platform sdk for windows xp sp2
set SPRINGSOURCE=h:\bin\games\taspring\taspring_0.73b1_src\taspring_0.73b1
set SPRINGAPPLICATION=h:\bin\games\taspring\application\taspring

set PATH=%PATH%;%VISUALCPPDIRECTORY%\bin

set CL=/EHsc /GR /D_WIN32_WINNT=0x0500 /D "WIN32" /D "_WINDOWS" /I"%PLATFORMSDK%\include" /I"%VISUALCPPDIRECTORY%\include" /I"%SPRINGSOURCE%\rts\System" /I"%SPRINGSOURCE%\rts"

set LINK=/LIBPATH:"%VISUALCPPDIRECTORY%\lib" /LIBPATH:"%PLATFORMSDK%\lib"

cl /MD /D BUILDING_SKIRMISH_AI /c myai.cpp

link /dll myai.obj myailoader.lib

copy /y myai.dll "%SPRINGAPPLICATION%"


