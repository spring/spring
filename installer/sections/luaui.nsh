
!ifdef INSTALL

  SetOutPath "$INSTDIR"
  File "gui.lua"
  File "usericons.tdf"

  SetOutPath "$INSTDIR\LuaUI"
  File /r /x .svn /x Config\*.lua "..\game\LuaUI\*.*"

  WriteRegDWORD HKCU "Software\SJ\spring" "LuaUI" "1"

!else

  Delete "$INSTDIR\gui.lua"
  Delete "$INSTDIR\usericons.tdf"
  RmDir /r "$INSTDIR\LuaUI"

!endif
