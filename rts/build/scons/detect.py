# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

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
		detected_platform='windows'
	elif sys.platform == 'darwin':
		print "Detected platform : darwin"
		print "WARNING: support is incomplete"
		detected_platform='darwin'
		print "NOTE: xcode files are available in /trunk/rts/build/xcode"
	else:
		print "Platform not supported yet, please edit SConstruct file"
		detected_platform='unknown'
	return detected_platform


# Set gcc_3_4 to false for versions of gcc below 3.4.
def processor(gcc_3_4=True):
	print "Detecting processor..."

	try:
		f = open('/proc/cpuinfo', 'r')
	except IOError:
		import platform
		bits = platform.architecture()[0]
		if bits == '32bit':
			# default to i686 on 32 bit platforms without /proc/cpuinfo
			print "  couldn't detect; assuming Pentium Pro or better"
			archflags=['-march=i686']
		else:
			print "  couldn't detect"
			archflags=[]
		return archflags

	str = f.readline()
	family=""
	model=-1
	vendor=""
	archflags=[]
	while str:
		if str.startswith("vendor_id"):
			if str.find("GenuineTMx86") != -1:
				# Transmeta
				vendor="GenuineTMx86"
			elif str.find("GenuineIntel") != -1:
				# Intel
				vendor="GenuineIntel"
			elif str.find("AuthenticAMD") != -1:
				# AMD
				vendor="AuthenticAMD"
		elif str.startswith("cpu"):
			# first re does not do anything more than family = str.strip() in my case
			family = re.sub('^[^0-9A-Za-z]*', '', str).strip()
			# this is more useful, finds the first block of numbers
			family_number = re.findall('[0-9]+', str)[0]
		elif str.startswith("model") and str.find("name") == -1:
			model = re.sub('^[^0-9]*', '', str).strip()
		if vendor == "GenuineTMx86":
			if str.startswith("model name"):
				if str.find("Transmeta Efficeon") != -1:
					# Automatically reorders/realigns when
					# converting to VLIW, so no alignment
					# saves space without sacrificing speed
					print "  found Transmeta Efficeon"
					archflags=['-march=i686']
					if gcc_3_4: archflags+=['-mtune=pentium3']
					else: archflags+=['-mcpu=pentium3']
					archflags+=['-msse2', '-mfpmath=sse', '-falign-functions=0', '-falign-jumps=0', '-falign-loops=0']
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
					if gcc_3_4: archflags=['-march=pentium-m']
					else:
						print "WARNING: gcc versions below 3.4 don't support -march=pentium-m, using -march=pentium4 instead"
						archflags=['-march=pentium4']
				elif str.find("Intel(R) Xeon(TM) CPU") != -1:
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
				elif str.find("Sempron") != -1:
					print "  found AMD Sempron 64"
					if gcc_3_4: archflags=['-march=athlon64']
					else:
						print "WARNING: gcc versions below 3.4 don't support -march=athlon64, using -march=athlon-4 -msse2 -mfpmath=sse instead"
						archflags=['-march=athlon', '-msse2', '-mfpmath=sse']
				elif (str.find("Athlon") != -1) or (str.find("Turion") != -1):
					if str.find("64") != -1:
						print "  found AMD64"
						if gcc_3_4: archflags=['-march=athlon64']
						else:
							print "WARNING: gcc versions below 3.4 don't support -march=athlon64, using -march=athlon-4 -msse2 -mfpmath=sse instead"
							archflags=['-march=athlon', '-msse2', '-mfpmath=sse']
					# maybe this should use int(family_number) rather than family
					# i have no way of checking
					elif family == 6 and model == 8:
						print "  found AMD Athlon Thunderbird XP"
						archflags=['-march=athlon-xp']
					else:
						print "  found AMD Athlon"
						archflags=['-march=athlon']
				elif str.find("Opteron") != -1:
					print "  found AMD Opteron"
					if gcc_3_4: archflags=['-march=opteron']
					else:
						print "WARNING: gcc versions below 3.4 don't support -march=opteron, using -march=athlon-4 -msse2 -mfpmath=sse instead"
						archflags=['-march=athlon-4', '-msse2', '-mfpmath=sse']
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
	if len(archflags) == 0:
		if vendor == "":
			if family != -1: # has to be checked...
				if family_number in ('970'):
					print "  found PowerPC 970 (G5)"
					archflags=['-mtune=G5', '-maltivec', '-mabi=altivec']
				#i think these are all the same as far as gcc cares
				elif family_number in ('7450','7455','7445','7447','7457'):
					print "  found PowerPC 7450 (G4 v2)"
					archflags=['-mtune=7450', '-maltivec', '-mabi=altivec']
				elif family_number in ('7400','7410'):
					print "  found PowerPC 7400 (G4)"
					archflags=['-mtune=7400', '-maltivec', '-mabi=altivec']
				elif family_number in ('750'):
					print "  found PowerPC 750 (G3)"
					archflags=['-mtune=750']
				elif family_number in ('740'):
					print "  found PowerPC 740 (G3)"
					archflags=['-mtune=740']
				elif family_number in ('604'):
					print "  found PowerPC 604"
					archflags=['-mtune=604']
				elif family_number in ('603'):
					print "  found PowerPC 603"
					archflags=['-mtune=603']



	f.close()
	if len(archflags) == 0:
		print "Couldn't detect your CPU, guessing i686 compatible.."
		archflags=['-march=i686']
	return archflags
