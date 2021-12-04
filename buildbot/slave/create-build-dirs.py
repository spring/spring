#!/usr/bin/env python2

import prepare
import os, subprocess, sys, shutil


CMAKEPARAM=[]
if 'CMAKEPARAM' in os.environ:
	CMAKEPARAM=os.environ['CMAKEPARAM'].split(" ")


print("Creating BUILDDIR: %s" %(prepare.BUILDDIR))
if not os.path.isdir(prepare.BUILDDIR):
	os.makedirs(prepare.BUILDDIR)

basedir = os.path.join(prepare.BUILDDIR, 'base')
if os.path.isdir(basedir):
	print("erasing old base content... %s" %(basedir))
	shutil.rmtree(basedir)

print("configuring %s with %s in %s" %(prepare.SOURCEDIR, CMAKEPARAM + sys.argv[3:], prepare.BUILDDIR))
subprocess.call(['cmake'] + CMAKEPARAM + sys.argv[3:] + [prepare.SOURCEDIR], cwd=prepare.BUILDDIR)
subprocess.call([prepare.MAKE, "generateSources", "-j1"], cwd=prepare.BUILDDIR)

