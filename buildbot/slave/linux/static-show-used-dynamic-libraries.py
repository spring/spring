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

