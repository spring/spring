#!/usr/bin/env python

import os, subprocess
import prepare

if not "MINGWLIBS_PATH" in os.environ:
	print('MINGWLIBS_PATH not set')
	exit(1)


MINGWLIBS_PATH = os.environ["MINGWLIBS_PATH"]
if not os.path.isdir(MINGWLIBS_PATH):
	print("cloning mingwlibs git-repo...")
	subprocess.call(["git", "clone", "git://github.com/spring/vclibs14.git", MINGWLIBS_PATH])


GITOUTPUT=subprocess.check_output(["git", "fetch"], cwd=MINGWLIBS_PATH)
print(GITOUTPUT)
if len(GITOUTPUT) > 0:
	print("removing builddir %s (mingwlibs updated)"%(prepare.BUILDDIR))
	shutil.rmtree(prepare.BUILDDIR)
	subprocess.call(["git","clean", "-f", "-d"], cwd=MINGWLIBS_PATH)
	subprocess.call(["git","reset", "--hard", "origin/master"], cwd=MINGWLIBS_PATH)


