#!/usr/bin/env python

## author:  Kloot
## date:    September 12, 2010
## purpose: UBB-colorizes lines from IRC/lobby logs

import random
import re
import sys
import time

LOG_LINE_PREFIX  = "[b][color=#%s]%s[/color][/b]"
DEV_NAME_PATTERN = re.compile("<[a-zA-Z0-9\[\]_]*>")
## Pattern for pidgin, this is an example line:
## (16:07:52) user: hello world!
## (server messages have to be cleaned by hand)
#DEV_NAME_PATTERN = re.compile(" [a-zA-Z0-9\[\]_]*: ")
DEV_NAME_LIST    = ["abma", "BrainDamage", "hoijui", "jK", "Kloot", "Tobi", "zerver"]
DEV_NAME_COLORS  = ["000000", "CC0000", "00BF40", "0040BF", "CC00CC", "DD6000", "00BFFF"]

def ReadLines(logName):
	try:
		logFile  = open(logName, 'r')
		logLines = logFile.read()
		logLines = logLines.split('\n')
		logFile  = logFile.close()
		return logLines
	except IOError:
		print "[ReadLines] cannot open file \"%s\" for reading" % logName
		return []

def WriteLines(logName, logLines):
	try:
		logFile = open(logName + ".colorized", 'w')
		logFile.write(logLines)
		logFile = logFile.close()
	except IOError:
		print "[WriteLines] cannot open file \"%s\" for writing " % logName

def ProcessLines(lines):
	random.shuffle(DEV_NAME_COLORS)

	prefix = ""
	nlines = ""
	onames = {}

	for line in lines:
		match = re.search(DEV_NAME_PATTERN, line)

		if (match == None):
			continue

		oname = match.group(0)
		oname = oname[1: -1]

		if (not onames.has_key(oname)):
			nname = oname

			for n in DEV_NAME_LIST:
				if (oname.find(n) >= 0):
					nname = n; break

			onames[oname] = (len(onames), nname)

		npair = onames[oname]
		color = (npair[0] < len(DEV_NAME_COLORS) and DEV_NAME_COLORS[npair[0]]) or "F00F00"
		nline = (LOG_LINE_PREFIX % (color, "<" + npair[1] + ">")) + line[match.span()[1]: ] + '\n'
		nlines += nline

	gmtime = time.gmtime()
	pcolor = "000000"
	prefix += "%s: %d-%d-%d\n" % (((LOG_LINE_PREFIX % (pcolor, "Date")), gmtime[2], gmtime[1], gmtime[0]))
	prefix += "%s: " % (LOG_LINE_PREFIX % (pcolor, "Present"))
	knames = onames.keys() 

	for k in xrange(len(knames)):
		oname = knames[k]
		npair = onames[oname]
		color = DEV_NAME_COLORS[npair[0]]
		prefix += (LOG_LINE_PREFIX % (color, npair[1]))
		prefix += (((k < len(knames) - 1) and ", ") or "")

	return (prefix + "\n\n" + nlines)

def Main(argc, argv):
	if (argc == 2):
		WriteLines(argv[1], ProcessLines(ReadLines(argv[1])))
	else:
		print "usage: python %s <log.txt>" % argv[0]
		print "writes to <log.txt>.colorized"

if (__name__ == "__main__"):
	Main(len(sys.argv), sys.argv)
