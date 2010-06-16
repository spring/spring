#!/usr/bin/env python
#
# This is meant for manual execution, and should be run whenever
# the content of cont/LuaUI/ changed.
# Use like this:
# python {this_script}.py > installer/sections/luaui.nsh
#
# @param baseDir for example: "/home/userX/src/spring"
#

################################################################################
#
# !ifdef INSTALL
#
#   SetOutPath "$INSTDIR"
#   File "${CONTENT_DIR}\luaui.lua"
# 
#   SetOutPath "$INSTDIR\LuaUI"
#   File /r /x .svn /x Config\*.lua "${CONTENT_DIR}\LuaUI\*.*"
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


if len(sys.argv) > 1:
	baseDir = sys.argv[1]
else:
	baseDir = os.path.join(sys.path[0], '..')

contentDirCandidates = ['cont']
luaUIDir = 'LuaUI'
contentBase = '${CONTENT_DIR}'
instBase = '$INSTDIR'

# Change to source root dir
os.chdir(sys.path[0])
os.chdir('..')

contentDir = ""
for d in contentDirCandidates:
	contentDir = d
	try:
		os.chdir(os.path.join(contentDir, luaUIDir))
		break
	except OSError:
		contentDir = ""

if contentDir == "":
	# Content directory not found
	sys.exit(1)
else:
	# Change to the content dir
	os.chdir(os.path.join(baseDir, contentDir))

################################################################################

def GetDirs(path, dirs):
	dirs[path] = []
	filelist = os.listdir(path)
	for f in filelist:
		fullname = os.path.join(path, f)
		if os.path.isdir(fullname):
			if (f != 'Config') and (f != '.git') and (f != 'base') and (f != 'freedesktop'):
				GetDirs(fullname, dirs)
		else:
			if (f != 'CMakeLists.txt'):
				dirs[path].append(f)


def toWinPath(path):
	return path.replace('/', '\\')

dirs = {}
GetDirs(luaUIDir, dirs)


################################################################################

print('!ifdef INSTALL')

print('')
print('  ; Purge old file from 0.75 install.')
print('  Delete "' + instBase + '\LuaUI\unitdefs.lua"')

print('')
print('  SetOutPath "' + instBase + '"')
print('  File "' + toWinPath(os.path.join(contentBase, 'luaui.lua')) + '"')
print('')
for d in dirs:
	print('  SetOutPath "' + toWinPath(os.path.join(instBase, d)) + '"')
	for f in dirs[d]:
		print('  File "' + toWinPath(os.path.join(contentBase, d, f)) + '"')
print('')

print('!else')

print('')
print('  Delete "' + toWinPath(os.path.join(instBase, 'luaui.lua')) + '"')
print('')
for d in dirs:
	for f in dirs[d]:
		print('  Delete "' + toWinPath(os.path.join(instBase, d, f)) + '"')
	print('  RmDir  "' + toWinPath(os.path.join(instBase, d)) + '"')
print('')
print('  RmDir  "' + toWinPath(os.path.join(instBase, luaUIDir)) + '"')
print('')

print('!endif')


################################################################################
