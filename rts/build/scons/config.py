# Copyright (C) 2006  Tobi Vollebregt
# Code copied from the old build system, but heavily reordered and/or rewritten.
# vim:noet ts=4 sts=4 sw=4

import os, re, sys

###############################
### configuration functions ###
###############################


def check_zip_version(env, conf):
	print "Checking for zip...",
	try:
		f = os.popen("zip -h 2>/dev/null")
		version = f.read()
	finally:
		f.close()
	if version:
		version = re.search('Zip [0-9]\.*[0-9]*\.*[0-9]*', version).group()
		print version, "found"
	else:
		print "not found"
		env.Exit(1)


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
	print "  Guessing", name, "include path..."
	if env['platform'] == 'windows':
		path = [os.path.join(os.path.join(env['mingwlibsdir'], 'include'), subdir)]
		path += [os.path.join(os.path.join(env['mingwlibsdir'], 'usr', 'include'), subdir)]
		path += [os.path.abspath(os.path.join(os.path.join(env['mingwlibsdir'], 'include'), subdir))]
		path += [os.path.abspath(os.path.join(os.path.join(env['mingwlibsdir'], 'usr', 'include'), subdir))]
		# Everything lives in mingwlibs anyway...
		if os.environ.has_key('MINGDIR'):
			path += [os.path.join(os.path.join(os.environ['MINGDIR'], 'include'), subdir)]
	elif env['platform'] == 'darwin':
		path = [os.path.join('/opt/local/include', subdir)]
	elif env['platform'] == 'freebsd':
		path = [os.path.join('/usr/local/include', subdir)]
	else:
		path = [os.path.join('/usr/include', subdir)]
	env.AppendUnique(CPPPATH = path)
	for f in path:
		print '\t\t', f

def check_freetype2(env, conf):
	print "Checking for Freetype2..."
	freetype = False
	""" I changed the order of the freetype-config and pkg-config checks
	because the latter returns a garbage freetype version on (at least) my system --tvo """

	# put this here for crosscompiling
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'Freetype2', 'freetype2')
		return
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
	# put this here for crosscompiling
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'SDL', 'SDL')
		return
	print "  Checking for sdl-config...",
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
	# put this here for crosscompiling
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'OpenAL', 'AL')
		return
	print "  Checking for openal-config...",
	openalcfg = env.WhereIs("openal-config")
	if openalcfg:
		print openalcfg
		env.ParseConfig(openalcfg+" --cflags --libs")
	else:
		print "not found"
		guess_include_path(env, conf, 'OpenAL', 'AL')


def check_ogg(env, conf):
	print "Checking for Ogg headers..."
	guess_include_path(env, conf, 'ogg', 'ogg')

def check_vorbis(env, conf):
	print "Checking for Vorbis(-file) headers..."
	guess_include_path(env, conf, 'vorbisfile', 'vorbis')



def check_python(env, conf):
	print "Checking for Python 2.6...",
	print
	guess_include_path(env, conf, 'Python', 'python2.6')
	print "Checking for Python 2.5...",
	print
	guess_include_path(env, conf, 'Python', 'python2.5')
	print "Checking for Python 2.4...",
	print
	guess_include_path(env, conf, 'Python', 'python2.4')


def check_java(env, conf):
	print "Checking for Java includes ...",
	if env.has_key('javapath') and env['javapath']:
		env.AppendUnique(CPPPATH = [env['javapath']])
		env.AppendUnique(CPPPATH = [os.path.join(env['javapath'], "linux")])
		return
	if env['platform'] == 'windows':
		guess_include_path(env, conf, 'Java', 'java')
		return
	if os.path.exists('/usr/local/jdk1.6.0/include'):
		env.AppendUnique(CPPPATH = '/usr/local/jdk1.6.0/include')
		env.AppendUnique(CPPPATH = '/usr/local/jdk1.6.0/include/' + env['platform'])
		return
	possible_dirs = []
	for root in ["/usr/local/lib/jvm", "/usr/local/lib64/jvm", "/usr/lib/jvm", "/usr/lib64/jvm", "/usr/java"]:
		if os.path.exists(root) and os.path.isdir(root):
			dirs = os.listdir(root)
			for dir in dirs:
				testdir = os.path.join(root, dir, "include")
				if dir[0] != '.' and os.path.exists(os.path.join(testdir, "jni.h")):
					possible_dirs += [testdir]
	if len(possible_dirs) == 0:
		print "not found"
		# FIXME TEST ON MINGW (ie make consistent with mingwlibs)
		guess_include_path(env, conf, 'Java', 'java')
	else:
		possible_dirs.sort()
		print possible_dirs[-1]
		env.AppendUnique(CPPPATH = [possible_dirs[-1]])
		env.AppendUnique(CPPPATH = [os.path.join(possible_dirs[-1], "linux")])

def check_java_bin(env, conf):
	print "Checking for Java executables ...",
	if (os.environ.has_key('JAVA_HOME')):
		env.AppendENVPath('PATH', os.path.join(os.environ['JAVA_HOME'], 'bin'))
		#env.AppendENVPath('LD_LIBRARY_PATH', os.environ['JAVA_HOME'] + '/jre/lib/i386/server/')


class Dependency:
	def __init__(self, libraries, headers):
		self.libraries = libraries
		self.headers = headers

	def __lt__(self, other):
		return (self.libraries + self.headers) < (other.libraries + other.headers)

	def CheckLibraries(self, conf):
		if len(self.libraries) != 0:
			succes = False
			for l in self.libraries:
				succes = conf.CheckLib(l)
				if succes: break
			if not succes:
				print "Could not find one of these libraries: " + str(self.libraries)
				return False
		return True

	def CheckHeaders(self, conf):
		if len(self.headers) != 0:
			succes = False
			for h in self.headers:
				succes = conf.CheckCXXHeader(h)
				if succes: break
			if not succes:
				print "Could not find one of these headers: " + str(self.headers)
				return False
		return True


def CheckHeadersAndLibraries(env, conf):
	print "\nChecking headers and libraries"

	boost_common = Dependency([], ['boost/cstdint.hpp'])
	boost_thread = Dependency(['boost_thread'], ['boost/thread.hpp'])
	boost_regex  = Dependency(['boost_regex'],   ['boost/regex.hpp'])
	boost_serial = Dependency([], ['boost/serialization/split_member.hpp'])
	boost_po     = Dependency(['boost_program_options'], ['boost/program_options.hpp'])
	boost_system  = Dependency(['boost_system'],   ['boost/system/error_code.hpp'])

	if env.Dictionary('CC').find('gcc') != -1: gcc = True
	else: gcc = False

	for boost in (boost_thread, boost_regex, boost_po, boost_system):
		l = boost.libraries[0]
		if gcc: boost.libraries = [l+'-gcc-mt', l+'-mt', l+'-gcc', l]
		else:   boost.libraries = [l+'-mt', l]

	d = [boost_common, boost_regex, boost_serial, boost_thread, boost_po, boost_system]

	d += [Dependency(['GL', 'opengl32'], ['GL/gl.h'])]
	d += [Dependency(['GLU', 'glu32'], ['GL/glu.h'])]
	d += [Dependency(['GLEW', 'glew32'], ['GL/glew.h'])]
	d += [Dependency(['zlib', 'zlib1', 'z'], ['zlib.h'])]
	d += [Dependency(['freetype6', 'freetype'], ['ft2build.h'])]
	d += [Dependency(['IL', 'devil'], ['IL/il.h'])]
	d += [Dependency(['ILU', 'ilu'], ['IL/ilu.h'])]
	d += [Dependency(['openal', 'openal32', 'OpenAL32'], ['AL/al.h'])]

	if env['platform'] == 'windows':
		d += [Dependency(['imagehlp'], [])]
		d += [Dependency(['gdi32'],    [])]
		d += [Dependency(['winmm'],    [])]
		d += [Dependency(['wsock32'],  [])]
		d += [Dependency(['ole32'],    [])]
		d += [Dependency(['mingw32'],  [])]
		d += [Dependency(['SDLmain'],  [])]
		d += [Dependency(['ws2_32'],  [])]
	else:
		d += [Dependency(['Xcursor'], ['X11/Xcursor/Xcursor.h'])]
		d += [Dependency(['X11'], ['X11/X.h'])]
		#d += [Dependency(['jvm'],     ['jni.h'])]

	d += [Dependency(['vorbisfile'], ['vorbis/vorbisfile.h'])]
	d += [Dependency(['vorbis'], [])]
	d += [Dependency(['ogg'], ['ogg/ogg.h'])]

	d += [Dependency(['SDL', 'SDL-1.1'], ['SDL/SDL.h', 'SDL11/SDL.h'])]
	d += [Dependency(['python2.6', 'python26', 'python2.5', 'python25',
		'python2.4', 'python24'], ['Python.h'])]
	d += [Dependency([], ['jni.h'])]

	if env['use_tcmalloc']:
		d += [Dependency(['tcmalloc'], [])]

	all_succes = True

	for c in d:
		if not c.CheckHeaders(conf) or not c.CheckLibraries(conf):
			all_succes = False

	if not all_succes:
		print "Not all tests finished succesfully.  You are probably missing one of the"
		print "build dependencies.  See config.log for details."
		env.Exit(1)


def configure(env, conf_dir):
	print "\nConfiguring spring"
	conf = env.Configure(conf_dir = conf_dir)
	# we dont have or need zip on windows..
	if env['platform'] != 'windows':
		check_zip_version(env, conf)
	check_debian_powerpc(env, conf)
	check_freetype2(env, conf)
	check_sdl(env, conf)

	check_openal(env, conf)

	check_ogg(env, conf)
	check_vorbis(env, conf)

	check_python(env, conf)
	check_java(env, conf)
	check_java_bin(env, conf)
	CheckHeadersAndLibraries(env, conf)
	env = conf.Finish()

	print "\nEverything seems OK.  Run `scons' now to build."
