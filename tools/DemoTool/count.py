#!/usr/bin/python
# counts cmds between NEWFRAMES and prints anything above 400
# usage: ./demotool -d /path/to/some/demo.sdf |./count.py
import sys
import fileinput

count = 0
for line in fileinput.input():
	args = line.strip().split(" ")
	if len(args)>1 and args[1] == "NEWFRAME":
		if count>400:
			print "%s %s" % (args[0], count)
		count = 0
		sumframes = 0
	else:
		count = count + 1
