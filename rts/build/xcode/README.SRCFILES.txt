Files not used on mac build
  --- File ---               --- Replacement ---
  AVIGenerator.cpp               -
  BackgroundReader.cpp       BackgroundReader.mm
  DllLib.cpp                 SoLib.cpp (linux)
  dotfileHandler.cpp         UserDefsHandler.mm
  GLXPBuffer.cpp             (FBO or [not written]MacPBuffer.mm)
  WinPBuffer.cpp
  HPIFile.cpp (not used anyway atm)
  iowin32.c
  LuaBinder.cpp (havn't dl'd and tested mac vers of lua yet)
  LuaFunctions.cpp
  main.m (not in svn...this will eventually be the main for mac)
  RegHandler.cpp             UserDefsHandler.mm
  Win32Cmd.cpp               PosixCmd.cpp (linux)
  Win32Support.cpp (win utilities not duplicated on mac or linux)
  DxSound.cpp

Files to compile as objective-c++ (with incorrect extension [.cpp])
(usually includes a cocoa or foundation header for mac spec. code)
  ConfigHandler.cpp

REMEMBER to tell xcode to remove all resources copying besides the info plist...the datafiles need to be copied seperately.
I'm using a straight copy cmd that pulls them into the build directory rather than the resources directory which i havn't added code to support properly yet.