# Copyright (C) 2006  Tobi Vollebregt
# Functions copied from the old build system.

import re, sys

########################################
### platform/cpu detection functions ###
########################################


def platform():
	if sys.platform == 'linux2' or sys.platform == 'linux-i386':
		print "Detected platform : linux"
		detected_platform='linux'
	elif sys.platform[:7] == 'freebsd':
		print "Detected platform : freebsd (%s)" % sys.platform
		print "WARNING: support is incomplete"
		detected_platform='freebsd'
	elif sys.platform == 'win32':
		print "Detected platform : windows"
		print "WARNING: support is incomplete"
		detected_platform='windows'
	else:
		print "Platform not supported yet, please edit SConstruct file"
		detected_platform='unknown'
	return detected_platform


def processor():
	#this function is unix,linux,freebsd only
	print "Detecting processor..."
	f = open('/proc/cpuinfo', 'r')
	str = f.readline()
	family=-1
	model=-1
	vendor=""
	while str:
		if str.startswith("vendor_id"):
			if str.find("GenuineTMx86") != -1:
				# Transmeta
				vendor="GenuineTMx86"
			elif str.find("GenuineIntel") != -1:
				# Intel
				vendor="GenuineIntel"
			elif str.find("AuthenticAMD"):
				# AMD
				vendor="AuthenticAMD"
		elif str.startswith("cpu "):
			family = re.sub('^[^0-9A-Za-z]*', '', str).strip()
		elif str.startswith("model") and str.find("name") == -1:
			model = re.sub('^[^0-9]*', '', str).strip()
		if vendor == "GenuineTMx86":
			if str.startswith("model name"):
				if str.find("Transmeta Efficeon") != -1:
					# Automatically reorders/realigns when
					# converting to VLIW, so no alignment
					# saves space without sacrificing speed
					print "  found Transmeta Efficeon"
					archflags=['-march=i686', '-mtune=pentium3', '-msse2', '-mfpmath=sse', '-falign-functions=0', '-falign-jumps=0', '-falign-loops=0']
		elif vendor == "GenuineIntel":
			if str.startswith("model name"):
				if str.find("Intel(R) Pentium(R) 4 CPU") != -1:
					print "  found Intel Pentium 4"
					archflags=['-march=pentium4']
				elif str.find("Coppermine") != -1:
					print "  found Intel Celeron (Coppermine)"
					archflags=['-march=pentium3']
				elif str.find("Pentium III") != -1:
					print "  found Intel Pentium III"
					archflags=['-march=pentium3']
				elif str.find("Pentium II") != -1:
					print "  found Intel Pentium II"
					archflags=['-march=pentium2']
				elif str.find("Intel(R) Celeron(R) CPU") != -1:
					print "  found Intel Celeron (Willamette)"
					archflags=['-march=pentium4']
				elif str.find("Celeron") != -1:
					print "  found Intel Celeron 1"
					archflags=['-march=pentium2']
				elif str.find("Intel(R) Pentium(R) M") != -1:
					print "  found Intel Pentium-M"
					archflags=['-march=pentium-m']
				elif str.find("Intel(R) Xeon(R) CPU") != -1:
					print "  found Intel Xeon w/EM64T"
					archflags=['-march=nocona', '-mmmx', '-msse3']
		elif vendor == "AuthenticAMD":
			if str.startswith("model name"):
				if str.find("Duron") != -1:
					if model == 7:
						print "  found AMD Duron Morgan"
						archflags=['-march=athlon-xp']
					elif model == 3:
						print "  found AMD Mobile Duron"
						archflags=['-march=athlon-tbird']
					else:
						print "  found AMD Duron"
						archflags=['-march=athlon-tbird']
				elif str.find("Athlon") != -1:
					if str.find("64") != -1:
						print "  found AMD Athlon 64"
						archflags=['-march=athlon64']
					elif family == 6 and model == 8:
						print "  found AMD Athlon Thunderbird XP"
						archflags=['-march=athlon-xp']
					else:
						print "  found AMD Athlon"
						archflags=['-march=athlon']
				elif str.find("Opteron") != -1:
					print "  found AMD Opteron"
					archflags=['-march=opteron']
		else:
			if str.find("Nehemiah") != -1:
				print "  found VIA Nehemiah"
				archflags=['-march=c3-2']
			elif str.find("Eden") != -1:
				print "  found VIA Eden C3"
				archflags=['-march=c3']
			elif str.find("Ezra") != -1:
				print "  found VIA Ezra"
				archflags=['-march=c3']

		str = f.readline()
	if not archflags:
		if vendor == "":
			if family.find("970"):
				print "  found PowerPC 970 (G5)"
				archflags=['-mtune=G5', '-maltivec', '-mabi=altivec']
			elif family.find("7450"):
				print "  found PowerPC 7450 (G4 v2)"
				archflags=['-mtune=7450', '-maltivec', '-mabi=altivec']
			elif family.find("7400"):
				print "  found PowerPC 7400 (G4)"
				archflags=['-mtune=7400', '-maltivec', '-mabi=altivec']
			elif family.find("750"):
				print "  found PowerPC 750 (G3)"
				archflags=['-mtune=750']
			elif family.find("604e"):
				archflags=['-mtune=604e']
		else:
			# unidentified x86
			archflags=['-march=i686']

	f.close()
	return archflags
