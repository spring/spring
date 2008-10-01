rem optional batch file to set up c++ paths before calling nant
rem You'll need to edit this with the path of Visual C++ Toolk 2003

set VCPPDIRECTORY=h:\bin\Microsoft Visual C++ Toolkit 2003

set PATH=%PATH%;%VCPPDIRECTORY%\bin
set CL=/I"%VCPPDIRECTORY%\include"
set LINK=/LIBPATH:"%VCPPDIRECTORY%\lib"

call nant
