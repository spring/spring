
!ifdef INSTALL

  SetOutPath "$INSTDIR"
  File "gui.lua"
  File "usericons.tdf"

  SetOutPath "$INSTDIR\LuaUI"
  File /r /x .svn /x Config\*.lua "..\game\LuaUI\*.*"

  WriteRegDWORD HKCU "Software\SJ\spring" "DWord value" 0x00000001

!else

  Delete "$INSTDIR\gui.lua"
  Delete "$INSTDIR\usericons.tdf"
  RmDir /r "$INSTDIR\LuaUI"

!endif
