#!/usr/bin/env python
#
# this scripts creates an nullsoft script that removes files for a given .7z archive
# python {this_script}.py > installer/sections/luaui.nsh
#
# @param baseDir for example: "/home/userX/src/spring"
#


from __future__ import print_function
import subprocess, sys

def uniquify(list):
    checked = []
    for e in list:
        if e not in checked:
            checked.append(e)
    return checked

def toWinPath(path):
	return path.replace('/', '\\')

def parseArgv(argv):
	prefix=""
	config = argv.split(":")
	if len(config)==2:
		prefix=config[1]
	return config[0], prefix

def sortPaths(argv):
	argv.sort(key=lambda x: x.count("\\"), reverse=True)
	return argv


def getContents(archives):
	"""
		parses the output of 7z for file and returns a list of files/paths found inside it
	"""
	paths=[]
	files=[]
	for archive in archives:
		path, prefix=parseArgv(archive)
		output=str(subprocess.check_output(["7z","l","-slt", path]))
		lines=output.split("\n")
		if (len(lines)==1): # python 3 hack
			lines = output.split("\\n")
		for line in lines:
			isdir=False
			if line.startswith('Path = '):
				path=line[7:]
			if line.startswith('Attributes = '):
				if line[13]=='D':
					paths.append(toWinPath(prefix+path))
				else:
					files.append(toWinPath(prefix+path))

	return uniquify(files), uniquify(paths)

def writeNsh(files, paths, argv):
	"""
		writes uninstall cmds for files + paths into the file nsh
	"""
	print("; This file is automaticly created, don't edit it!")
	command="make_uninstall_nsh.py"
	for arg in argv:
		command=command+" " +arg

	print("; created with: %s" % command)
	print(";")
	for file in files:
		print('Delete "$INSTDIR\%s"'%(file))
	for path in paths:
		print('RmDir "$INSTDIR\%s"'%(path))
	for archive in argv:
		path, prefix = parseArgv(archive)
		if prefix!="":
			#strip backslash at the end
			if prefix.endswith("\\"):
				prefix=prefix[:-1]
			print('RmDir "$INSTDIR\%s"'%(prefix))


if len(sys.argv)<2:
	print("Usage %s [<7z archive>[:<subpath to extract]]+"%(sys.argv[0]), file=sys.stderr)
	sys.exit(1)
else:
	files, paths = getContents(sys.argv[1:])
	writeNsh(files, sortPaths(paths), sys.argv[1:])

