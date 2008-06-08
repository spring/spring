#!/usr/bin/env python

################################################################################
#
# !ifdef INSTALL
#
#   SetOutPath "$INSTDIR"
#   File "..\game\luaui.lua"
# 
#   SetOutPath "$INSTDIR\LuaUI"
#   File /r /x .svn /x Config\*.lua "..\game\LuaUI\*.*"
# 
# !else
#
#   Delete "$INSTDIR\luaui.lua"
#   RmDir /r "$INSTDIR\LuaUI"
#
# !endif
#
################################################################################


import os

try:
	os.chdir('game')
except OSError:
	os.chdir('../game')

top = 'LuaUI/'


################################################################################

def GetDirs(path, dirs):
  dirs[path] = []
  filelist = os.listdir(path)
  for f in filelist:
    fullname = path + f
    if os.path.isdir(fullname):
      if (f != 'Config') and (f != '.svn'):
        fullname = fullname + '/'
        GetDirs(fullname, dirs)
    else:
      dirs[path].append(f)


def osName(path):
  return path.replace('/', '\\')

dirs = {}
GetDirs(top, dirs)

#for d in dirs:
#  for f in dirs[d]:
#    print(d, f)


################################################################################

print('!ifdef INSTALL')

print('')
print('  SetOutPath "$INSTDIR"')
print('  File "..\\game\\luaui.lua"')
print('')
for d in dirs:
  print('  SetOutPath "$INSTDIR\\' + osName(d) + '"')
  for f in dirs[d]:
    print('  File "..\\game\\' + osName(d) + osName(f) + '"')
print('')

print('!else')

print('')
print('  Delete "$INSTDIR\\luaui.lua"')
print('')
for d in dirs:
  for f in dirs[d]:
    print('  Delete "$INSTDIR\\' + osName(d) + osName(f) + '"')
  print('  RmDir "$INSTDIR\\' + osName(d) + '"')
print('')
print('  RmDir "$INSTDIR\\LuaUI"')
print('')

print('!endif')


################################################################################
