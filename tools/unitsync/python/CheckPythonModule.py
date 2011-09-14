#! /usr/bin/env python
import sys
try:
	 X = __import__(sys.argv[1]) 
except:
	sys.exit(1)
sys.exit(0)
