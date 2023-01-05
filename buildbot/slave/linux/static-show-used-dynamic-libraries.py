#!/usr/bin/python3

import sys
import os
import subprocess

executable=sys.argv[1]

assert os.path.isfile(executable), executable

env = os.environ.copy()
env["LC_ALL"] = "C"

output = subprocess.check_output(["readelf", "-d", executable], universal_newlines=True, env=env)

allowed_libs = (
	#unitsync + spring"
	"libdl.so.2",
	"libm.so.6",
	"libpthread.so.0",
	"libc.so.6",
	"ld-linux-x86-64.so.2",
	# spring
	"libSDL2-2.0.so.0",
	"libGL.so.1",
	"libXcursor.so.1",
	"libX11.so.6",
	"libopenal.so.1",
)
libs = []
match = 'Shared library: '
for line in output.split("\n"):
	if not match in line:
		continue
	libs.append(line.split(match)[1].strip("[]"))

exitcode = 0
for lib in libs:
	if not lib in allowed_libs:
		print("linked to not whitelisted lib:", lib)
		exitcode = 1

sys.exit(exitcode)

