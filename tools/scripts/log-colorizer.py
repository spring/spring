## author:  Kloot
## date:    September 12, 2010
## purpose: UBB-colorizes lines from IRC/lobby logs

import random
import re
import sys

LOG_LINE_PREFIX  = "[b][color=#%s]<%s>[/color][/b]"
DEV_NAME_PATTERN = re.compile("<[a-zA-Z0-9\[\]_]*>")
DEV_NAME_LIST    = ["abma", "BrainDamage", "hoijui", "jK", "Kloot", "Tobi", "zerver"]
DEV_NAME_COLORS  = ["000000", "FF0000", "00FF00", "0000FF", "FF00FF", "FFFF00", "00FFFF"]

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
		color = (npair[0] < len(DEV_NAME_COLORS) and DEV_NAME_COLORS[ npair[0] ]) or "F00F00"
		nname = npair[1]
		nline = (LOG_LINE_PREFIX % (color, nname)) + line[match.span()[1]: ] + '\n'

		nlines += nline

	return nlines

def Main(argc, argv):
	if (argc == 2):
		WriteLines(argv[1], ProcessLines(ReadLines(argv[1])))
	else:
		print "usage: python %s <log.txt>" % argv[0]

if (__name__ == "__main__"):
	Main(len(sys.argv), sys.argv)
