#!/usr/bin/env python
#
# this scripts creates an nullsoft script that removes files for a given .7z archive
# python {this_script}.py > installer/sections/luaui.nsh
#
# @param baseDir for example: "/home/userX/src/spring"
#


import subprocess, sys

def uniquify(list):
    checked = []
    for e in list:
        if e not in checked:
            checked.append(e)
    return checked

def toWinPath(path):
	return path.replace('/', '\\')

def getContents(archives):
	"""
		parses the output of 7z for file and returns a list of files/paths found inside it
	"""
	paths=[]
	files=[]
	for archive in archives:
		output=subprocess.check_output(["7z","l","-slt", archive])
		lines=output.split("\n")
		for line in lines:
			isdir=False
			if line.startswith('Path = '):
				path=line[7:]
			if line.startswith('Attributes = '):
				if line[13]=='D':
					paths.append(toWinPath(path))
				else:
					files.append(toWinPath(path))

	return uniquify(files), uniquify(paths)

def writeNsh(files, paths):
	"""
		writes uninstall cmds for files + paths into the file nsh
	"""
	print "; This file is automaticly created, don't edit it!"
	for file in files:
		print 'Delete "%s"'%(file)
	for path in paths:
		print 'RmDir "%s"'%(path)

if len(sys.argv)<2:
	print "Usage %s [<7z archive>]+"%(sys.argv[0])
else:
	files, paths = getContents(sys.argv[1:])
	writeNsh(files, paths)

