#!/usr/bin/env python
#
# @param distributionDir
#

################################################################################
#
# !ifdef INSTALL
#
#   SetOutPath "$INSTDIR"
#   File "$DIST_DIR\luaui.lua"
# 
#   SetOutPath "$INSTDIR\LuaUI"
#   File /r /x .svn /x Config\*.lua "$DIST_DIR\LuaUI\*.*"
# 
# !else
#
#   Delete "$INSTDIR\luaui.lua"
#   RmDir /r "$INSTDIR\LuaUI"
#
# !endif
#
################################################################################


import os, sys


distDirCandidates = [sys.argv[1], 'dist', 'game', 'cont']

distDir = ""
for d in distDirCandidates:
	distDir = d
	try:
		os.chdir(distDir)
		break
	except OSError:
		distDir = os.path.join('..', distDir)
		try:
			os.chdir(distDir)
			break
		except OSError:
			distDir = ""
			continue

# Distribution directory not found
if distDir == "":
	sys.exit(1);

top = 'LuaUI/'


################################################################################

def GetDirs(path, dirs):
  dirs[path] = []
  filelist = os.listdir(path)
  for f in filelist:
    fullname = path + f
    if os.path.isdir(fullname):
      if (f != 'Config') and (f != '.git'):
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
print('  File "$DIST_DIR\\luaui.lua"')
print('')
for d in dirs:
  print('  SetOutPath "$INSTDIR\\' + osName(d) + '"')
  for f in dirs[d]:
    print('  File "$DIST_DIR\\' + osName(d) + osName(f) + '"')
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
