#!/usr/bin/env python

import prepare
import os, subprocess, sys

BUILDDIR=os.environ['BUILDDIR']
CMAKEPARAM=os.environ['CMAKEPARAM']


print("Creating BUILDDIR: %s" %(BUILDDIR))
if not os.path.isdir(BUILDDIR):
	os.makedirs(BUILDDIR)

print("configuring %s with %s %s ..." %(prepare.SOURCEDIR, CMAKEPARAM, sys.argv[3:]))

os.chdir(BUILDDIR)
subprocess.call(['cmake', CMAKEPARAM ] + sys.argv[3:] + [prepare.SOURCEDIR])

print("erasing old base content...")
if os.path.isdir("base"):
	os.removedirs("base")

subprocess.call([prepare.MAKE, "generateSources"])

