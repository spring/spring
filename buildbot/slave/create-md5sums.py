#!/usr/bin/env python

import hashlib, sys, os

if len(sys.argv) <= 2:
	print("usage: %s dir [file suffixes]+" %(sys.argv[0]))
	print("recursively (!!!) creates .md5 files for all files with provided")
	exit(1)
def hashfile(filename, blocksize=4096):
	hasher = hashlib.md5()
	f = open(filename, "rb")
	buf = f.read(blocksize)
	while len(buf) > 0:
		hasher.update(buf)
		buf = f.read(blocksize)
	return hasher.hexdigest()

def createhash(filename, md5):
	f = open(md5, 'wb')
	hexdigest = hashfile(filename)
	f.write("%s %s" %(hexdigest, os.path.basename(filename)))
	f.close()
	return hexdigest

def verify(fn, md5):
	f = open(md5, "rb").read()
	hexdigest, filename = f.split(" ", 1)
	if filename != os.path.basename(fn):
		print("Invalid file: %s %s"% (filename, fn))
		return False, ""
	h = hashfile(fn)
	return h == hexdigest, h

def handlefile(f, suffixes, suffix):
	if f.endswith(suffix):
		return
	skip = True

	basename = os.path.basename(f)

	for s in suffixes: # check if filename matches suffix
		if basename.endswith(s):
			skip = False
			break
	if skip: return

	md5file = f + suffix
	if os.path.isfile(md5file):
		res, h = verify(f, md5file)
		print("Verifying %s: %s - %s" % (h, f, res))
	else:
		h = createhash(f, md5file)
		print("Creating %s: %s" % (h, md5file))


path = sys.argv[1]
suffixes = sys.argv[2:]
suffix = ".md5"


for root, directories, filenames in os.walk(path):
	for filename in filenames:
		handlefile(os.path.join(root, filename), suffixes, suffix)

