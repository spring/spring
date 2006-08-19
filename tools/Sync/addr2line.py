#!/usr/bin/python
#
# Author: Tobi Vollebregt
#
# Simple tool which reads input and passes all addresses in it (which must be
# in format as written by sync debugger) to 'addr2line -e spring'.
#
# It then replaces the address in the input line with the first line of output
# of addr2line, effectively resolving hexadecimal addresses to filename:lineno
# pairs, and outputs the input line.
#
# Any line which doesn't have the right format is just copied to stdout.
#
# Usage: ./addr2line.py [filename]
# If filename is given it reads from that file, otherwise it reads from stdin.


import os,sys


# Need to buffer the input because addr2line takes very long to load (the large
# spring executable) but is really quick in resolving multiple addresses.

def flush_buffer(buffer):
	global exename
	if not exename:
		for s in buffer:
			if s[-2:-1] == ']':
				exename = s[s.find('./'):s.find('(')]
				if exename:
					# print 'addr2line: Executable name:', exename
					break
		# Just dump the buffer to stdout if we can't find an exename
		if not exename:
			for s in buffer:
				print s,
			return
	# Pass all hex addresses to addr2line and read back the results.
	cmd = 'addr2line -e ' + exename
	for s in buffer:
		if s[-2:-1] == ']':
			cmd += ' '+s[s.rfind('[')+1:-2]
	p = os.popen(cmd)
	for s in buffer:
		if s[-2:-1] == ']':
			sloc = p.readline()
			s = s[0:s.rfind('[')+1] + sloc[0:-1] + ']\n'
		print s,
	p.close()


if len(sys.argv) == 2:
	filename = sys.argv[1]
else:
	filename = '/dev/stdin'

exename = None
buffer = []
f = file(filename)
while True:
	s = f.readline()
	if len(s) == 0:
		break
	buffer += [s]
	if len(buffer) == 2000:
		flush_buffer(buffer)
		buffer = []
f.close()
flush_buffer(buffer)
