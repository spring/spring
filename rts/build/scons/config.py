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
		if os.environ.has_key('INCLUDE'):   # MSVC
			path = os.environ['INCLUDE']
		elif os.environ.has_key('MINGDIR'): # MinGW
			path = os.path.join(os.environ['MINGDIR'], 'include')
		else:
			path = 'c:/mingw/include'
	elif env['platform'] == 'darwin':
		path = '/opt/local/include'
	else:
		path = '/usr/include'
	path = os.path.join(path, subdir)
	env.AppendUnique(CPPPATH = [path])
	print path


def check_freetype2(env, conf):
	print "Checking for Freetype2..."
	freetype = False
	""" I changed the order of the freetype-config and pkg-config checks
	because the latter returns a garbage freetype version on (at least) my system --tvo """

	print "  Checking for freetype-config...",
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
	sdlcfg = env.WhereIs("sdl-config")
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
		guess_include_path(env, conf, 'SDL', 'SDL')


def check_openal(env, conf):
	print "Checking for OpenAL..."
	print "  Checking for openal-config...",
	openalcfg = env.WhereIs("openal-config")
	if openalcfg:
		print openalcfg
		env.ParseConfig(openalcfg+" --cflags --libs")
	else:
		print "not found"
		guess_include_path(env, conf, 'OpenAL', 'AL')


def check_headers(env, conf):
	print "\nChecking header files"

	if not conf.CheckCHeader('ft2build.h'):
		print "Freetype2 headers are required for this program"
		env.Exit(1)
	if not conf.CheckCHeader('SDL/SDL.h'):
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
	if not conf.CheckCXXHeader('boost/filesystem/path.hpp'):
		print ' Cannot find Boost filesystem headers'
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
	if not env['disable_lua']:
		env.AppendUnique(CPPPATH = ['../lua/luabind', '../lua/lua/include'])
		if not conf.CheckCXXHeader('luabind/luabind.hpp'):
			print ' Cannot find Luabind header'
			env.Exit(1)
		if not conf.CheckCXXHeader('lua.h'):
			print ' Cannot find Lua header'
			env.Exit(1)


def check_libraries(env, conf):
	print "\nChecking libraries"

	if env.Dictionary('CC').find('gcc') != -1: gcc = True
	else: gcc = False

	if not conf.CheckLib("freetype"):
		print "Freetype2 is required for this program"
		env.Exit(1)
	if not conf.CheckLib('SDL'):
		print 'LibSDL is required for this program'
		env.Exit(1)
	if not conf.CheckLib('openal'):
		print 'OpenAL is required for this program'
		env.Exit(1)
	if not (conf.CheckLib('GLEW') or conf.CheckLib('glew32')):
		print "You need GLEW to compile this program"
		env.Exit(1)

	def check_boost_library(lib, hdr):
		l = 'boost_' + lib
		if not ((gcc and conf.CheckLib(l+'-gcc-mt')) or conf.CheckLib(l+'-mt') or
				(gcc and conf.CheckLib(l+'-gcc'))    or conf.CheckLib(l)):
			print "You need the Boost " + lib + " library for this program"
			env.Exit(1)

	check_boost_library('filesystem', 'boost/filesystem/path.hpp')
	check_boost_library('thread', 'boost/thread.hpp')
	check_boost_library('regex', 'boost/regex.hpp')

	if not conf.CheckLib('IL'):
		print "You need the DevIL image library for this program"
		env.Exit(1)

	if env['use_tcmalloc']:
		if not conf.CheckLib('tcmalloc'):
			print "tcmalloc from goog-perftools requested but not available"
			print "falling back to standard malloc"
			env['use_tcmalloc'] = False


def configure(env, conf_dir):
	print "\nChecking configure scripts"
	conf = env.Configure(conf_dir = conf_dir)
	if env['platform'] == 'darwin':
		# Hackery to support Darwinports configure scripts
		env['ENV']['PATH'] = os.environ['PATH']
	check_debian_powerpc(env, conf)
	check_freetype2(env, conf)
	check_sdl(env, conf)
	check_openal(env, conf)
	check_headers(env, conf)
	check_libraries(env, conf)
	env = conf.Finish()
	print "\nEverything seems OK.  Run `scons' now to build."
