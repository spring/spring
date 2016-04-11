#!/usr/bin/env python

# creates or verifies .md5 files in specified dir

import hashlib, sys, os

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
	assert(filename == fn)
	h = hashfile(fn)
	return h == hexdigest, h

if len(sys.argv) <= 1:
	print("usage: %s dir" %(sys.argv[0]))
	exit(1)

path = sys.argv[1]
suffix = ".md5"

files = { f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f)) }

for f in files:
	if f.endswith(suffix):
		continue
	md5file = f + suffix
	if md5file in files:
		res, h = verify(f, md5file)
		print("Verifying %s: %s - %s" % (h, f, res))
	else:
		h = createhash(f, md5file)
		print("Creating %s: %s" % (h, md5file))
