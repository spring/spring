# Copyright (C) 2006  Tobi Vollebregt
# Code copied from the old build system, but heavily reordered and/or rewritten.

import os, re, sys

###############################
### configuration functions ###
###############################


def check_gcc_version(env, conf = None):
	# The GCC version is needed by the detect.processor() function to recognize
	# versions below 3.4 and versions equal to or greater than 3.4.
	# As opposed to the other functions in this file, which are called from the
	# configure() function below, this one is called directly from rts.py while
	# parsing `optimize=' configure argument.
	print env.subst("Checking $CC version..."),
	try:
		f = os.popen(env.subst('$CC --version'))
		version = f.read()
	finally:
		f.close()
	if version:
		version = re.search('[0-9]\.[0-9]\.[0-9]', version).group()
		print version
		return re.split('\.', version)
	else:
		print env.subst("$CC not found")
		env.Exit(1)


def check_debian_powerpc(env, conf):
	if 'Debian' in sys.version:
		# Figure out if we're running on PowerPC
		# and which version of GCC we're dealing with.
		try:
			f = os.popen("echo '' | gcc -E -dM -")
			predefines = f.read()
		finally:
			f.close()
		if predefines:
			if '#define __powerpc__ 1' in predefines:
				if '#define __GNUC__ 4' in predefines:
					old_gpp = env['CXX']
					env['CXX'] = "g++-3.3"
					if not conf.TryCompile('int main(void) { return 0; } ', '.cpp'):
						env['CXX'] = old_gpp
						print "Warning: Compiling with GCC4 on Debian PowerPC. The resulting executable may corrupt the stack before main()."
					else:
						env['CC'] = "gcc-3.3"
						print "Note: Compiling on Debian PowerPC. Switched to GCC 3."


def guess_include_path(env, conf, name, subdir):
	print "  Guessing", name, "include path...",
	if env['platform'] == 'windows':
		path = [os.path.join(os.path.join('mingwlibs', 'include'), subdir)]
		if os.environ.has_key('MINGDIR'):
			path += [os.path.join(os.path.join(os.environ['MINGDIR'], 'include'), subdir)]
	elif env['platform'] == 'darwin':
		path = [os.path.join('/opt/local/include', subdir)]
	else:
		path = [os.path.join('/usr/include', subdir)]
	env.AppendUnique(CPPPATH = path)
	for f in path:
		print f,
	print ''

def check_freetype2(env, conf):
	print "Checking for Freetype2..."
	freetype = False
	""" I changed the order of the freetype-config and pkg-config checks
	because the latter returns a garbage freetype version on (at least) my system --tvo """

	print "  Checking for freetype-config...",
	# put this here for crosscompiling
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'Freetype2', 'freetype2')
		return
	freetypecfg = env.WhereIs("freetype-config")
	if freetypecfg:
		print freetypecfg
		ftv = conf.TryAction(freetypecfg+" --ftversion")
		if ftv[0]:
			cmd = freetypecfg+" --ftversion"
			fcmd = freetypecfg+" --cflags --libs"
			freetype = True
		else:
			print "  "+freetypecfg+" does not give library version"
	else:
		print "not found"

	if not freetype:
		print "  Checking for pkg-config...",
		pkgcfg = env.WhereIs("pkg-config")
		if pkgcfg:
			print pkgcfg
			print "  Checking for Freetype2 package... ",
			ret = conf.TryAction(pkgcfg+" --exists freetype2")
			if ret[0]:
				print "found"
				cmd = pkgcfg+" --modversion freetype2"
				fcmd = pkgcfg+" --libs --cflags freetype2"
				freetype = True
			else:
				print "not found"
		else:
			print "not found"

	if freetype:
		print "  Checking for Freetype >= 2.0.0...",
		ftobj = os.popen(cmd)
		ftver = ftobj.read()
		fterr = ftobj.close()
		print ftver,
		if ftver.split('.') >= ['2','0','0']:
			env.ParseConfig(fcmd)
		else:
			print "You need Freetype version 2.0.0 or greater for this program"
			env.Exit(1)
	else:
		guess_include_path(env, conf, 'Freetype2', 'freetype2')


def check_sdl(env, conf):
	print "Checking for SDL..."
	print "  Checking for sdl-config...",
	# put this here for crosscompiling
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'SDL', 'SDL')
		return
	sdlcfg = env.WhereIs("sdl-config")
	if not sdlcfg: # for FreeBSD
		sdlcfg = env.WhereIs("sdl11-config")
	if sdlcfg:
		print sdlcfg
		print "  Checking for LibSDL >= 1.2.0...",
		sdlobj = os.popen(sdlcfg+" --version")
		sdlver = sdlobj.read()
		sdlerr = sdlobj.close()
		print sdlver,
		if sdlver.split('.') >= ['1','2','0']:
			env.ParseConfig(sdlcfg+" --cflags --libs")
		else:
			print "You need LibSDL version 1.2.0 or greater for this program"
			env.Exit(1)
	else:
		print "not found"
		if env['platform'] == 'freebsd':
			guess_include_path(env, conf, 'SDL', 'SDL11')
		else:
			guess_include_path(env, conf, 'SDL', 'SDL')


def check_openal(env, conf):
	print "Checking for OpenAL..."
	print "  Checking for openal-config...",
	# put this here for crosscompiling
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'OpenAL', 'AL')
		return
	openalcfg = env.WhereIs("openal-config")
	if openalcfg:
		print openalcfg
		env.ParseConfig(openalcfg+" --cflags --libs")
	else:
		print "not found"
		guess_include_path(env, conf, 'OpenAL', 'AL')


def check_python(env, conf):
	print "Checking for Python 2.4...",
	print ""
	guess_include_path(env, conf, 'Python', 'python2.4')


def check_headers(env, conf):
	print "\nChecking header files"

	if not conf.CheckCHeader('zlib.h'):
		print "Zlib headers are required for this program"
		env.Exit(1)
	if not conf.CheckCHeader('ft2build.h'):
		print "Freetype2 headers are required for this program"
		env.Exit(1)
	# second check for FreeBSD.
	if not conf.CheckCHeader('SDL/SDL.h') and not conf.CheckCHeader('SDL11/SDL.h'):
		print 'LibSDL headers are required for this program'
		env.Exit(1)
	if not conf.CheckCHeader('AL/al.h'):
		print 'OpenAL headers are required for this program'
		env.Exit(1)
	if not conf.CheckCHeader('GL/gl.h'):
		print 'OpenGL headers are required for this program'
		env.Exit(1)
	if not conf.CheckCHeader('GL/glu.h'):
		print 'OpenGL utility (glu) headers are required for this program'
		env.Exit(1)
	if not conf.CheckCHeader('GL/glew.h'):
		print ' Cannot find GLEW http://glew.sourceforge.net'
		env.Exit(1)
	if not conf.CheckCXXHeader('boost/cstdint.hpp'):
		print ' Boost library must be installed'
		env.Exit(1)
	if not conf.CheckCXXHeader('boost/thread.hpp'):
		print ' Cannot find Boost threading headers'
		env.Exit(1)
	if not conf.CheckCXXHeader('boost/regex.hpp'):
		print ' Cannot find Boost regex header'
		env.Exit(1)
	if not conf.CheckCXXHeader('boost/spirit.hpp'):
		print ' Cannot find Boost Spirit header'
		env.Exit(1)
	if not conf.CheckCHeader('IL/il.h'):
		print ' Cannot find DevIL image library header'
		env.Exit(1)


def check_libraries(env, conf):
	print "\nChecking libraries"

	if env.Dictionary('CC').find('gcc') != -1: gcc = True
	else: gcc = False

	if not conf.CheckLib('GL') and not conf.CheckLib('opengl32'):
		print "You need an OpenGL development library for this program"
		env.Exit(1)
	if not conf.CheckLib('GLU') and not conf.CheckLib('glu32'):
		print "You need the OpenGL Utility (GLU) library for this program"
		env.Exit(1)
	if not conf.CheckLib("z"):
		print "Zlib is required for this program"
		env.Exit(1)
	if not conf.CheckLib("freetype"):
		print "Freetype2 is required for this program"
		env.Exit(1)
	# second check for Windows.
	if not conf.CheckLib('openal') and not conf.CheckLib('openal32'):
		print 'OpenAL is required for this program'
		env.Exit(1)

	#FIXME unitsync doesn't compile on mingw, also this breaks current mingwlibs when crosscompiling..
	# second check for Windows.
	if not conf.CheckLib('python2.4') and not conf.CheckLib('python24'):
		print 'python is required for this program'
		env.Exit(1)

	# second check for Windows.
	if not (conf.CheckLib('GLEW') or conf.CheckLib('glew32')):
		print "You need GLEW to compile this program"
		env.Exit(1)

	def check_boost_library(lib, hdr):
		l = 'boost_' + lib
		if not ((gcc and conf.CheckLib(l+'-gcc-mt')) or conf.CheckLib(l+'-mt') or
				(gcc and conf.CheckLib(l+'-gcc'))    or conf.CheckLib(l)):
			print "You need the Boost " + lib + " library for this program"
			env.Exit(1)

	check_boost_library('thread', 'boost/thread.hpp')
	check_boost_library('regex', 'boost/regex.hpp')

	# second check for Windows.
	if not conf.CheckLib('IL') and not conf.CheckLib('devil') and not conf.CheckLib('DevIL'):
		print "You need the DevIL image library for this program"
		env.Exit(1)

	if env['platform'] == 'windows':
		if not conf.CheckLib('dsound'):
			print "On windows you need the dsound library for this program"
			env.Exit(1)
		if not conf.CheckLib('gdi32'):
			print "On windows you need the gdi32 library for this program"
			env.Exit(1)
		if not conf.CheckLib('winmm'):
			print "On windows you need the winmm library for this program"
			env.Exit(1)
		if not conf.CheckLib('wsock32'):
			print "On windows you need the wsock32 library for this program"
			env.Exit(1)
		if not conf.CheckLib('ole32'):
			print "On windows you need the ole32 library for this program"
			env.Exit(1)
		if not conf.CheckLib('mingw32'):
			print "On windows you need the mingw32 library for this program"
			env.Exit(1)
		if not conf.CheckLib('SDLmain'):
			print "On windows you need the SDLmain library for this program"
			env.Exit(1)

	# second check for FreeBSD.
	if not conf.CheckLib('SDL') and not conf.CheckLib('SDL-1.1'):
		print 'LibSDL is required for this program'
		env.Exit(1)

	if env['use_tcmalloc']:
		if not conf.CheckLib('tcmalloc'):
			print "tcmalloc from goog-perftools requested but not available"
			print "falling back to standard malloc"
			env['use_tcmalloc'] = False


def configure(env, conf_dir):
	print "\nChecking configure scripts"
	conf = env.Configure(conf_dir = conf_dir)
	check_debian_powerpc(env, conf)
	check_freetype2(env, conf)
	check_sdl(env, conf)
	if env['platform'] != 'windows':
		check_openal(env, conf)
	check_python(env, conf)
	check_headers(env, conf)
	check_libraries(env, conf)
	env = conf.Finish()
	print "\nEverything seems OK.  Run `scons' now to build."
