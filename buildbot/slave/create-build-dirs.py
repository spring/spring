#!/usr/bin/env python

import prepare
import os, subprocess, sys


CMAKEPARAM=[]
if 'CMAKEPARAM' in os.environ:
	CMAKEPARAM=os.environ['CMAKEPARAM'].split(" ")


print("Creating BUILDDIR: %s" %(prepare.BUILDDIR))
if not os.path.isdir(prepare.BUILDDIR):
	os.makedirs(prepare.BUILDDIR)

print("configuring %s with %s ..." %(prepare.SOURCEDIR, CMAKEPARAM + sys.argv[3:]))

os.chdir(prepare.BUILDDIR)
subprocess.call(['cmake'] + CMAKEPARAM + sys.argv[3:] + [prepare.SOURCEDIR])

print("erasing old base content...")
if os.path.isdir("base"):
	os.removedirs("base")

subprocess.call([prepare.MAKE, "generateSources"])

