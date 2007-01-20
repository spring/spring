
!ifdef INSTALL

  SetOutPath "$INSTDIR"
  File "..\game\gui.lua"
  File "..\game\usericons.tdf"

  SetOutPath "$INSTDIR\LuaUI"
  File /r /x .svn /x Config\*.lua "..\game\LuaUI\*.*"

!else

  Delete "$INSTDIR\gui.lua"
  Delete "$INSTDIR\usericons.tdf"
  RmDir /r "$INSTDIR\LuaUI"

!endif
