# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

import os, sys
import platform
import SCons.Util
from SCons.Options import Options
import config, detect, filelist


def exists(env):
	return True


# HACK   Oh noes, msvcrt doesn't support arbitrary lenght commandlines :/
# source: http://www.scons.org/cgi-sys/cgiwrap/scons/moin.cgi/LongCmdLinesOnWin32
def fix_windows_spawn(env):
	# for cross compilation
	if sys.platform != 'win32':
		return

	if env['platform'] == 'windows':
		import win32file
		import win32event
		import win32process
		import win32security
		import string

		def my_spawn(sh, escape, cmd, args, spawnenv):
			for var in spawnenv:
				spawnenv[var] = spawnenv[var].encode('ascii', 'replace')

			sAttrs = win32security.SECURITY_ATTRIBUTES()
			StartupInfo = win32process.STARTUPINFO()
			newargs = string.join(map(escape, args[1:]), ' ')
			cmdline = cmd + " " + newargs

			# check for any special operating system commands
			if cmd == 'del':
				for arg in args[1:]:
					win32file.DeleteFile(arg)
				exit_code = 0
			else:
				# otherwise execute the command.
				hProcess, hThread, dwPid, dwTid = win32process.CreateProcess(None, cmdline, None, None, 1, 0, spawnenv, None, StartupInfo)
				win32event.WaitForSingleObject(hProcess, win32event.INFINITE)
				exit_code = win32process.GetExitCodeProcess(hProcess)
				win32file.CloseHandle(hProcess);
				win32file.CloseHandle(hThread);
				return exit_code
		env['SPAWN'] = my_spawn


def generate(env):
	# Fix scons & gcc borkedness (scons not looking in PATH for gcc
	# and mingw gcc 4.1 linker crashing if TMP or TEMP isn't set).
	env['ENV']['PATH'] = os.environ['PATH']
	if os.environ.has_key('MINGDIR'):
		env['ENV']['MINGDIR'] = os.environ['MINGDIR']
	if os.environ.has_key('TMP'):
		env['ENV']['TMP'] = os.environ['TMP']
	if os.environ.has_key('TEMP'):
		env['ENV']['TEMP'] = os.environ['TEMP']

	#parse cmdline
	def makeHashTable(args):
		table = { }
		for arg in args:
			if len(arg) > 1:
				lst = arg.split('=')
				if len(lst) < 2: continue
				key = lst[0]
				value = lst[1]
				if len(key) > 0 and len(value) > 0: table[key] = value
		return table

	args = makeHashTable(sys.argv)

	# We break unitsync if the filename of the shared object has a 'lib' prefix.
	# It is also nicer for the AI shared objects.
	env['LIBPREFIX'] = ''

	# I don't see any reason to make this configurable --tvo.
	# Note that commenting out / setting this to `None' will break the buildsystem.
	env['builddir'] = '#build'
	if args.has_key('builddir'):
		env['builddir'] = args['builddir']
	bd = filelist.getAbsDir(env, env['builddir'])

	# SCons chokes in env.SConsignFile() if path doesn't exist.
	if not os.path.exists(bd):
		os.makedirs(bd)

	# Avoid spreading .sconsign files everywhere - keep this line
	env.SConsignFile(os.path.join(bd, 'scons_signatures'))

	usrcachefile = os.path.join(bd, 'usropts.py')
	intcachefile = os.path.join(bd, 'intopts.py')
	usropts = Options(usrcachefile)
	intopts = Options(intcachefile)

	#user visible options
	usropts.AddOptions(
		#permanent options
		('platform',          'Set to linux, freebsd or windows', None),
		('gml',               'Set to yes to enable the OpenGL Multithreading Library', False),
		('gmlsim',            'Set to yes to enable parallel threads for Sim/Draw', False),
		('gmldebug',          'Set to yes to enable GML call debugging', False),
		('debug',             'Set to yes to produce a binary with debug information', 0),
		('debugdefines',      'Set to no to suppress DEBUG and _DEBUG preprocessor #defines (use to add symbols to release build)', True),
		('syncdebug',         'Set to yes to enable the sync debugger', False),
		('synccheck',         'Set to yes to enable sync checker & resyncer', True),
		('synctrace',         'Enable sync tracing', False),
		('optimize',          'Enable processor optimizations during compilation', 1),
		('arch',              'CPU architecture to use', 'auto'),
		('profile',           'Set to yes to produce a binary with profiling information', False),
		('profile_generate',  'Set to yes to compile with -fprofile-generate to generate profiling information', False),
		('profile_use',       'Set to yes to compile with -fprofile-use to use profiling information', False),
		('ai_interfaces',     'Which AI Interfaces (and AIs using them) to build [all|native|java|none]', 'all'),
		('cpppath',           'Set path to extra header files', []),
		('libpath',           'Set path to extra libraries', []),
		('fpmath',            'Set to 387 or SSE on i386 and AMD64 architectures', 'sse'),
		('prefix',            'Install prefix used at runtime', '/usr/local'),
		('installprefix',     'Install prefix used for installion', '$prefix'),
		('builddir',          'Build directory, used at build-time', '#build'),
		('mingwlibsdir',      'MinGW libraries dir', '#mingwlibs'),
		('datadir',           'Data directory (relative to prefix)', 'share/games/spring'),
		('bindir',            'Directory for executables (rel. to prefix)', 'games'),
		('libdir',            'Directory for AI plugin modules (rel. to prefix)', 'lib/spring'),
		('strip',             'Discard symbols from the executable (only when neither debugging nor profiling)', False),
		#porting options - optional in a first phase
		('disable_avi',       'Set to no to turn on avi support', 'False on windows, True otherwise'),
		#other ported parts
		('use_tcmalloc',      'Use tcmalloc from goog-perftools for memory allocation', False),
		('use_nedmalloc',     'Use nedmalloc for memory allocation', False),
		('use_mmgr',          'Use memory manager', False),
		('use_gch',           'Use gcc precompiled header', True),
		('dc_allowed',        'Specifies whether FPS mode (Direct Control) is allowed in game', True),
		('cachedir',          'Cache directory (see scons manual)', None))

	#internal options
	intopts.AddOptions(
		('LINKFLAGS',     'linker flags'),
		('LIBPATH',       'library path'),
		('LIBS',          'libraries'),
		('CCFLAGS',       'c compiler flags'),
		('CXXFLAGS',      'c++ compiler flags'),
		('CPPDEFINES',    'c preprocessor defines'),
		('CPPPATH',       'c preprocessor include path'),
		('CC',            'c compiler'),
		('CXX',           'c++ compiler'),
		('RANLIB',        'ranlib'),
		('AR',            'ar'),
		('spring_defines','extra c preprocessor defines for spring'),
		('streflop_extra','extra options for streflop Makefile'),
		('is_configured', 'configuration version stamp'))

	usropts.Update(env)
	intopts.Update(env)

	env.Help(usropts.GenerateHelpText(env))

	# make the build dir globally absolute
	env['builddir'] = bd

	# Use this to avoid an error message 'how to make target configure ?'
	env.Alias('configure', None)

	if not 'configure' in sys.argv and not ((env.has_key('is_configured') and env['is_configured'] == 8) or env.GetOption('clean')):
		print "Not configured or configure script updated.  Run `scons configure' first."
		print "Use `scons --help' to show available configure options to `scons configure'."
		env.Exit(1)

	# Dont throw an exception if scons -c is run before scons configure (this is done by debian build system for example)
	if not env.has_key('is_configured') and env.GetOption('clean'):
		print "Not configured: nothing to clean"
		env.Exit(0)

	if 'configure' in sys.argv:

		# be paranoid, unset existing variables
		for key in ['platform', 'gml', 'gmlsim', 'gmldebug', 'debug', 'optimize',
			'profile', 'profile_use', 'profile_generate', 'cpppath',
			'libpath', 'prefix', 'installprefix', 'builddir',
			'mingwlibsdir', 'datadir', 'bindir', 'libdir',
			'cachedir', 'strip', 'disable_avi', 'use_tcmalloc',
			'use_nedmalloc', 'use_mmgr', 'use_gch', 'ai_interfaces',
			'LINKFLAGS', 'LIBPATH', 'LIBS', 'CCFLAGS',
			'CXXFLAGS', 'CPPDEFINES', 'CPPPATH', 'CC', 'CXX',
			'is_configured', 'spring_defines', 'arch']:
			if env.has_key(key): env.__delitem__(key)

		print "\nNow configuring.  If something fails, consult `config.log' for details.\n"

		env['is_configured'] = 8

		if args.has_key('platform'): env['platform'] = args['platform']
		else: env['platform'] = detect.platform()
		fix_windows_spawn(env)

		if os.environ.has_key('CC'):
			env['CC'] = os.environ['CC']
		else:
			env['CC'] = 'gcc'
		if os.environ.has_key('CXX'):
			env['CXX'] = os.environ['CXX']
		else:
			env['CXX'] = 'g++'

		# select proper tools for win crosscompilation by default
		is_crosscompiling = env['platform'] == 'windows' and os.name != 'nt'
		if os.environ.has_key('AR'):
			env['AR'] = os.environ['AR']
		elif is_crosscompiling:
			env['AR'] = 'i586-mingw32msvc-ar'

		if os.environ.has_key('RANLIB'):
			env['RANLIB'] = os.environ['RANLIB']
		elif is_crosscompiling:
			env['RANLIB'] = 'i586-mingw32msvc-ranlib'

		gcc_version = config.check_gcc_version(env)

		print 'Toolchain options:'
		print 'CC=%s' % env['CC']
		print 'CXX=%s' % env['CXX']
		print 'AR=%s' % env['AR']
		print 'RANLIB=%s' % env['RANLIB']
		print

		# Declare some helper functions for boolean and string options.
		def bool_opt(key, default):
			if args.has_key(key):
				if args[key] == 'no' or args[key] == 'false' or args[key] == '0':
					env[key] = False
				elif args[key] == 'yes' or args[key] == 'true' or args[key] == '1':
					env[key] = True
				else:
					print "\ninvalid", key, "option, must be one of: yes, true, no, false, 0, 1."
					env.Exit(1)
			else: env[key] = default

		def string_opt(key, default):
			if args.has_key(key):
				env[key] = args[key]
			else: env[key] = default

		def stringarray_opt(key, default):
			if args.has_key(key):
				env[key] = args[key].split(';')
			else: env[key] = default

		# Use single precision constants only.
		# This should be redundant with the modifications done by tools/double_to_single_precision.sed.
		# Other options copied from streflop makefiles.
		env['CCFLAGS'] = ['-fsingle-precision-constant', '-frounding-math', '-fsignaling-nans', '-mieee-fp']

		# set architecture
		if 'arch' in args and args['arch'] != 'auto':
			arch = args['arch']
			if not arch or arch == 'none':
				print 'Configuring for default architecture'
				marchFlag = ''
			else:
				print 'Configuring for', arch
				marchFlag = '-march=' + arch
		else:
			bits, archname = platform.architecture()
			if bits == '32bit' or env['platform'] == 'windows':
				print 'Configuring for i686'
				marchFlag = '-march=i686'
			else:
				print 'Configuring for default architecture'
				marchFlag = ''
		env['CCFLAGS'] += [marchFlag]
		env['streflop_extra'] = [marchFlag]

		# profile?
		bool_opt('profile', False)
		if env['profile']:
			print "profiling enabled,",
			env.AppendUnique(CCFLAGS=['-pg'], LINKFLAGS=['-pg'])
		else:
			print "profiling NOT enabled,",

		# debug?
		gcc_warnings = [
				'-Wchar-subscripts',
				'-Wformat=2',
				'-Winit-self',
				'-Wimplicit',
				'-Wmissing-braces',
				'-Wparentheses',
				'-Wsequence-point',
				'-Wreturn-type',
				'-Wswitch',
				'-Wtrigraphs',
				'-Wunused',
				'-Wuninitialized',
				'-Wunknown-pragmas'
			]
		if args.has_key('debug'):
			level = args['debug']
			if level == 'no' or level == 'false': level = '0'
			elif level == 'yes' or level == 'true': level = '3'
		else:
			level = '0'
		if int(level) == 0:
			print "debugging NOT enabled,",
			env['debug'] = 0
		elif int(level) >= 1 and int(level) <= 3:
			print "level", level, "debugging enabled,",
			env['debug'] = level
			# MinGW gdb chokes on the dwarf debugging format produced by '-ggdb',
			# so use the more generic '-g' instead.
			# MinGW 4.2.1 gdb does not like the DWARF2 debug format generated by default,
			# so produce STABS instead
			if env['platform'] == 'windows' or env['syncdebug']:
				env.AppendUnique(CCFLAGS=['-gstabs'])
			else:
				env.AppendUnique(CCFLAGS=['-ggdb'+level])
			# We can't enable -Wall because that silently enables an order of
			# initialization warning for initializers in constructors that
			# can not be disabled. (and it takes days to fix them all in code)
			env.AppendUnique(CFLAGS=gcc_warnings, CCFLAGS=gcc_warnings)
			if not args.has_key('debugdefines') or not args['debugdefines']:
				env.AppendUnique(CPPDEFINES=['DEBUG', '_DEBUG', 'NO_CATCH_EXCEPTIONS'])
			else:
				env.AppendUnique(CPPDEFINES=['NDEBUG'])
		else:
			print "\ninvalid debug option, must be one of: yes, true, no, false, 0, 1, 2, 3."
			env.Exit(1)

		if args.has_key('debugdefines') and args['debugdefines']:
			env.AppendUnique(CPPDEFINES=
					['DEBUG', '_DEBUG',
						'NO_CATCH_EXCEPTIONS'],
					CFLAGS=gcc_warnings, CCFLAGS=gcc_warnings)

		# optimize?
		if args.has_key('optimize'):
			level = args['optimize']
			if level == 'no' or level == 'false': level = '0'
			elif level == 'yes' or level == 'true': level = '2'
		else:
			if env['debug']: level = '0'
			else: level = '2'
		if level == 's' or level == 'size' or (int(level) >= 1 and int(level) <= 3):
			print "level", level, "optimizing enabled"
			env['optimize'] = level
			#archflags = detect.processor(gcc_version >= ['3','4','0'])
			# -fstrict-aliasing causes constructs like:
			#    float f = 10.0f; int x = *(int*)&f;
			# to break.
			# Since those constructs are used in the netcode and MathTest code, we disable the optimization.
			env.AppendUnique(CCFLAGS=['-O'+level, '-pipe', '-fno-strict-aliasing'])
			# MinGW 4.2 compiled binaries insta crash with this on...
			#if int(level) <= 2:
			#	env.AppendUnique(CCFLAGS=['-finline-functions','-funroll-loops'])
		elif int(level) == 0:
			print "optimizing NOT enabled",
			env['optimize'] = 0
		else:
			print "\ninvalid optimize option, must be one of: yes, true, no, false, 0, 1, 2, 3, s, size."
			env.Exit(1)

		# Generate profiling information? (for profile directed optimization)
		bool_opt('profile_generate', False)
		if env['profile_generate']:
			print "build will generate profiling information"
			env.AppendUnique(CCFLAGS=['-fprofile-generate'], LINKFLAGS=['-fprofile-generate'])

		# Use profiling information? (for profile directed optimization)
		bool_opt('profile_use', False)
		if env['profile_use']:
			print "build will use profiling information"
			env.AppendUnique(CCFLAGS=['-fprofile-use'], LINKFLAGS=['-fprofile-use'])

		# Must come before the '-fvisibility=hidden' code.
		bool_opt('syncdebug', False)
		bool_opt('synccheck', True)
		bool_opt('synctrace', False)
		string_opt('fpmath', 'sse')

		# If sync debugger is on, disable inlining, as it makes it much harder to follow backtraces.
		if env['syncdebug']:
			# Disable all possible inlining, just to be sure.
			env['CCFLAGS'] += ['-fno-default-inline', '-fno-inline', '-fno-inline-functions', '-fno-inline-functions-called-once']

		# It seems only gcc 4.0 and higher supports this.
		if gcc_version >= ['4','0','0'] and env['platform'] != 'windows':
			env['CCFLAGS'] += ['-fvisibility=hidden']

		# Allow easy switching between 387 and SSE fpmath.
		if env['fpmath']:
			env['CCFLAGS'] += ['-mfpmath='+env['fpmath']]
			env['streflop_extra'] += ['-mfpmath='+env['fpmath']]
			if env['fpmath'] == '387':
				print "WARNING: SSE math vs X87 math is unsynced!"
				print "WARNING: Do not go online with the binary you are currently building!"
			else:
				env['CCFLAGS'] += ['-msse']
				env['streflop_extra'] += ['-msse']

		env['CXXFLAGS'] = env['CCFLAGS']

		# Do not do this anymore because it may severely mess up our carefully selected options.
		# Print a warning and ignore them instead.
		# fall back to environment variables if neither debug nor optimize options are present
		if not args.has_key('debug') and not args.has_key('optimize'):
			if os.environ.has_key('CFLAGS'):
				#print "using CFLAGS:", os.environ['CFLAGS']
				#env['CCFLAGS'] = SCons.Util.CLVar(os.environ['CFLAGS'])
				print "WARNING: attempt to use environment CFLAGS has been ignored."
			if os.environ.has_key('CXXFLAGS'):
				#print "using CXXFLAGS:", os.environ['CXXFLAGS']
				#env['CXXFLAGS'] = SCons.Util.CLVar(os.environ['CXXFLAGS'])
				print "WARNING: attempt to use environment CXXFLAGS has been ignored."
			#else:
			#	env['CXXFLAGS'] = env['CCFLAGS']

		# nedmalloc crashes horribly when crosscompiled
		# on mingw gcc 4.2.1
		nedmalloc_default = False

		bool_opt('gml', False)
		bool_opt('gmlsim', False)
		bool_opt('gmldebug', False)
		bool_opt('strip', False)
		bool_opt('disable_avi', env['platform'] != 'windows')
		bool_opt('use_tcmalloc', False)
		bool_opt('use_nedmalloc', nedmalloc_default)
		bool_opt('use_mmgr', False)
		bool_opt('use_gch', True)
		bool_opt('dc_allowed', True)
		string_opt('prefix', '/usr/local')
		string_opt('installprefix', '$prefix')
		string_opt('builddir', '#build'),
		string_opt('mingwlibsdir', '#mingwlibs'),
		string_opt('datadir', 'share/games/spring')
		string_opt('bindir', 'games')
		string_opt('libdir', 'lib/spring')
		string_opt('cachedir', None)
		string_opt('ai_interfaces', 'all')

		# Make a list of preprocessor defines.
		env.AppendUnique(CPPDEFINES = ['_REENTRANT', '_SZ_ONE_DIRECTORY'])
		spring_defines = []

		if env['use_gch']:
			env.AppendUnique(CXXFLAGS = ['-DUSE_PRECOMPILED_HEADER'])
			print 'Precompiled header enabled'
		else:
			print 'Precompiled header disabled'

		spring_defines += ['_WIN32_WINNT=0x500']

		# gml library
		if env['gml']:
			env.AppendUnique(CCFLAGS = ['-mno-tls-direct-seg-refs'], CXXFLAGS = ['-mno-tls-direct-seg-refs'], LINKFLAGS = ['-mno-tls-direct-seg-refs'])		
			spring_defines += ['USE_GML']
			print 'OpenGL Multithreading Library is enabled'
			if env['gmlsim']:
				spring_defines += ['USE_GML_SIM']
				print 'Parallel threads for Sim/Draw is enabled'
				if env['gmldebug']:
					spring_defines += ['USE_GML_DEBUG']
					print 'GML call debugging is enabled'
				else:
					print 'GML call debugging is NOT enabled'
			else:
				print 'Parallel threads for Sim/Draw is NOT enabled'
		else:
			print 'OpenGL Multithreading Library and parallel threads for Sim/Draw are NOT enabled'

		# Add define specifying type of floating point math to use.
		if env['fpmath']:
			if env['fpmath'] == 'sse':
				spring_defines += ['STREFLOP_SSE']
			if env['fpmath'] == '387':
				spring_defines += ['STREFLOP_X87']

		# Add/remove SYNCDEBUG to enable/disable sync debugging.
		if env['syncdebug']:
			spring_defines += ['SYNCDEBUG']
		if env['synccheck']:
			spring_defines += ['SYNCCHECK']
		if env['synctrace']:
			spring_defines += ['TRACE_SYNC']

		# Don't define this: it causes a full recompile when you change it, even though it is only used in Main.cpp,
		# and some AIs maybe.  Just make exceptions in SConstruct.
		#defines += ['SPRING_DATADIR="\\"'+env['datadir']+'\\""']
		if not env['disable_avi']  : spring_defines += ['AVI_CAPTURING']
		if env['use_mmgr']         : spring_defines += ['USE_MMGR']
		if env['dc_allowed']       : spring_defines += ['DIRECT_CONTROL_ALLOWED']

		env['spring_defines'] = spring_defines

		stringarray_opt('cpppath', [])
		stringarray_opt('libpath', [])

		include_path = env['cpppath'] + ['#rts', '#rts/System']
		include_path += ['#rts/lib/lua/include', '#rts/lib/streflop']
		lib_path = env['libpath']

		if env['platform'] == 'freebsd':
			include_path += ['/usr/local/include', '/usr/X11R6/include', '/usr/X11R6/include/GL']
			lib_path += ['/usr/local/lib', '/usr/X11R6/lib']
			env.AppendUnique(CCFLAGS = ['-pthread'], CXXFLAGS = ['-pthread'])
		elif env['platform'] == 'linux':
			include_path += ['/usr/include', '/usr/include/GL']
			env.AppendUnique(CCFLAGS = ['-pthread'], CXXFLAGS = ['-pthread'], LINKFLAGS = ['-Wl,-E'])
		elif env['platform'] == 'darwin':
			include_path += ['/usr/include', '/usr/local/include', '/opt/local/include', '/usr/X11R6/include']
			lib_path += ['/opt/local/lib', '/usr/local/lib']
			env['SHLINKFLAGS'] = '$LINKFLAGS -dynamic'
			env['SHLIBSUFFIX'] = '.dylib'
		elif env['platform'] == 'windows':
			include_path += [os.path.join(env['mingwlibsdir'], 'include')]
			lib_path += [os.path.join(env['mingwlibsdir'], 'lib')]
			lib_path += [os.path.join(env['mingwlibsdir'], 'dll')]
			include_path += [os.path.abspath(os.path.join(env['mingwlibsdir'], 'include'))]
			lib_path += [os.path.abspath(os.path.join(env['mingwlibsdir'], 'lib'))]
			lib_path += [os.path.abspath(os.path.join(env['mingwlibsdir'], 'dll'))]
			if os.environ.has_key('MINGDIR'):
				include_path += [os.path.join(os.environ['MINGDIR'], 'include')]
				lib_path += [os.path.join(os.environ['MINGDIR'], 'lib')]
				lib_path += [os.path.join(os.environ['MINGDIR'], 'dll')]
			else:
				print 'ERROR: MINGDIR environment variable not set and MSVC build unsupported.'
				print 'Set it to your Dev-Cpp or MinGW install directory (e.g. C:\\Dev-Cpp) and try again.'
				env.Exit(1)
			env.AppendUnique(CCFLAGS = ['-mthreads'], CXXFLAGS = ['-mthreads'], LINKFLAGS = ['-mwindows', '-mthreads'])
		# use '-pthreads' for Solaris, according to /usr/include/boost/config/requires_threads.hpp

		env.AppendUnique(CPPPATH=include_path, LIBPATH=lib_path)

		config.configure(env, conf_dir=os.path.join(env['builddir'], 'sconf_temp'))

		usropts.Save(usrcachefile, env)
		intopts.Save(intcachefile, env)

	# make the prefix absolute
	env['prefix'] = filelist.getAbsDir(env, env['prefix'])

	# Substitute prefix in installprefix, and make installprefix absolute
	env['installprefix'] = filelist.getAbsDir(env, env.subst(env['installprefix']))

	# Fix up some suffices for mingw crosscompile.
	if env['platform'] == 'windows':
		env['SHLIBSUFFIX'] = '.dll'
		env['PROGSUFFIX'] = '.exe'

	#Should we strip the exe?
	if env.has_key('strip') and env['strip'] and not env['debug'] and not env['profile'] and not env.GetOption('clean'):
		env['strip'] = True
	else:
		env['strip'] = False

	#BuildDir support code
	if env['builddir']:
		for d in filelist.list_directories(env, 'rts'):
			env.BuildDir(os.path.join(env['builddir'], d), d, duplicate = False)
		for d in filelist.list_directories(env, 'AI'):
			env.BuildDir(os.path.join(env['builddir'], d), d, duplicate = False)

	#CacheDir support code
	if env.has_key('cachedir') and env['cachedir']:
		if not os.path.exists(env['cachedir']):
			os.makedirs(env['cachedir'])
		env.CacheDir(env['cachedir'])

	fix_windows_spawn(env)
