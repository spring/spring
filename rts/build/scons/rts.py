# Copyright (C) 2006  Tobi Vollebregt

import os, sys
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

	# We break unitsync if the filename of the shared object has a 'lib' prefix.
	# It is also nicer for the AI shared objects.
	env['LIBPREFIX'] = ''

	# I don't see any reason to make this configurable --tvo.
	# Note that commenting out / setting this to `None' will break the buildsystem.
	env['builddir'] = 'build'

	# SCons chokes in env.SConsignFile() if path doesn't exist.
	if not os.path.exists(env['builddir']):
		os.makedirs(env['builddir'])

	# Avoid spreading .sconsign files everywhere - keep this line
	# Use os.path.abspath() here because somehow the argument to SConsignFile is relative to the
	# directory of the toplevel trunk/SConstruct and not the current directory, trunk/rts/SConstruct.
	env.SConsignFile(os.path.abspath(os.path.join(env['builddir'], 'scons_signatures')))

	usrcachefile = os.path.join(env['builddir'], 'usropts.py')
	intcachefile = os.path.join(env['builddir'], 'intopts.py')
	usropts = Options(usrcachefile)
	intopts = Options(intcachefile)

	#user visible options
	usropts.AddOptions(
		#permanent options
		('platform',          'Set to linux, freebsd or windows', None),
		('debug',             'Set to yes to produce a binary with debug information', 0),
		('syncdebug',         'Set to yes to enable the sync debugger', False),
		('synccheck',         'Set to yes to enable sync checker & resyncer', False),
		('optimize',          'Enable processor optimizations during compilation', 1),
		('profile',           'Set to yes to produce a binary with profiling information', False),
		('cpppath',           'Set path to extra header files', []),
		('libpath',           'Set path to extra libraries', []),
		('fpmath',            'Set to 387 or SSE on i386 and AMD64 architectures', '387'),
		('prefix',            'Install prefix used at runtime', '/usr/local'),
		('installprefix',     'Install prefix used for installion', '$prefix'),
		('datadir',           'Data directory (relative to prefix)', 'share/games/spring'),
		('bindir',            'Directory for executables (rel. to prefix)', 'games'),
		('libdir',            'Directory for AI plugin modules (rel. to prefix)', 'lib/spring'),
		('strip',             'Discard symbols from the executable (only when neither debugging nor profiling)', True),
		#porting options - optional in a first phase
		('disable_avi',       'Set to no to turn on avi support', True),
		#other ported parts
		('use_tcmalloc',      'Use tcmalloc from goog-perftools for memory allocation', False),
		('use_mmgr',          'Use memory manager', False),
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
		('spring_defines','extra c preprocessor defines for spring'),
		('is_configured', 'configuration version stamp'))

	usropts.Update(env)
	intopts.Update(env)

	env.Help(usropts.GenerateHelpText(env))

	# Use this to avoid an error message 'how to make target configure ?'
	env.Alias('configure', None)

	if not 'configure' in sys.argv and not ((env.has_key('is_configured') and env['is_configured'] == 5) or env.GetOption('clean')):
		print "Not configured or configure script updated.  Run `scons configure' first."
		print "Use `scons --help' to show available configure options to `scons configure'."
		env.Exit(1)

	if 'configure' in sys.argv:

		# be paranoid, unset existing variables
		for key in ['platform', 'debug', 'optimize', 'profile', 'cpppath', 'libpath', 'prefix', 'installprefix', 'datadir', 'bindir', 'libdir', 'cachedir', 'strip', 'disable_avi', 'use_tcmalloc', 
			'use_mmgr', 'LINKFLAGS', 'LIBPATH', 'LIBS', 'CCFLAGS', 'CXXFLAGS', 'CPPDEFINES', 'CPPPATH', 'CC', 'CXX', 'is_configured', 
			'spring_defines']:
			if env.has_key(key): env.__delitem__(key)

		print "\nNow configuring.  If something fails, consult `config.log' for details.\n"

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

		env['is_configured'] = 5

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

		gcc_version = config.check_gcc_version(env)

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

		# profile?
		bool_opt('profile', False)
		if env['profile']:
			print "profiling enabled,",
			env.AppendUnique(CCFLAGS=['-pg'], LINKFLAGS=['-pg'])
		else:
			print "profiling NOT enabled,",

		# debug?
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
			if env['platform'] == 'windows' or env['syncdebug']:
				env.AppendUnique(CCFLAGS=['-g'], CPPDEFINES=['DEBUG', '_DEBUG'])
			else:
				env.AppendUnique(CCFLAGS=['-ggdb'+level], CPPDEFINES=['DEBUG', '_DEBUG'])
		else:
			print "\ninvalid debug option, must be one of: yes, true, no, false, 0, 1, 2, 3."
			env.Exit(1)

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
		elif int(level) == 0:
			print "optimizing NOT enabled",
			env['optimize'] = 0
		else:
			print "\ninvalid optimize option, must be one of: yes, true, no, false, 0, 1, 2, 3, s, size."
			env.Exit(1)

		# Must come before the '-fvisibility=hidden' code.
		bool_opt('syncdebug', False)
		bool_opt('synccheck', False)
		if env['syncdebug'] and env['synccheck']:
			print "syncdebug and synccheck are mutually exclusive. Please choose one."
			env.Exit(1)
		string_opt('fpmath', '387')

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
			if env['fpmath'] == 'sse':
				print "WARNING: SSE math vs X87 math is unsynced!"
				print "WARNING: Do not go online with the binary you are currently building!"
				env['CCFLAGS'] += ['-msse', '-msse2']

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

		bool_opt('strip', True)
		bool_opt('disable_avi', True)
		bool_opt('use_tcmalloc', False)
		bool_opt('use_mmgr', False)
		string_opt('prefix', '/usr/local')
		string_opt('installprefix', '$prefix')
		string_opt('datadir', 'share/games/spring')
		string_opt('bindir', 'games')
		string_opt('libdir', 'lib/spring')
		string_opt('cachedir', None)

		# Make a list of preprocessor defines.
		env.AppendUnique(CPPDEFINES = ['_REENTRANT', '_SZ_ONE_DIRECTORY'])
		spring_defines = ['DIRECT_CONTROL_ALLOWED']

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

		# Don't define this: it causes a full recompile when you change it, even though it is only used in Main.cpp,
		# and some AIs maybe.  Just make exceptions in SConstruct.
		#defines += ['SPRING_DATADIR="\\"'+env['datadir']+'\\""']
		if env['disable_avi']      : spring_defines += ['NO_AVI']
		if env['use_mmgr']         : spring_defines += ['USE_MMGR']

		env['spring_defines'] = spring_defines

		stringarray_opt('cpppath', [])
		stringarray_opt('libpath', [])

		include_path = env['cpppath'] + ['rts', 'rts/System']
		include_path += ['lua/luabind', 'lua/lua/include']
		lib_path = env['libpath'] + ['rts/lib/streflop']

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
			include_path += [os.path.join('mingwlibs', 'include')]
			lib_path += [os.path.join('mingwlibs', 'lib')]
			if os.environ.has_key('MINGDIR'):
				include_path += [os.path.join(os.environ['MINGDIR'], 'include')]
				lib_path += [os.path.join(os.environ['MINGDIR'], 'lib')]
			else:
				print 'ERROR: MINGDIR environment variable not set and MSVC build unsupported.'
				print 'Set it to your Dev-Cpp or MinGW install directory (e.g. C:\\Dev-Cpp) and try again.'
				env.Exit(1)
			env.AppendUnique(CCFLAGS = ['-mthreads'], CXXFLAGS = ['-mthreads'], LINKFLAGS = ['-mwindows'])
		# use '-pthreads' for Solaris, according to /usr/include/boost/config/requires_threads.hpp

		env.AppendUnique(CPPPATH=include_path, LIBPATH=lib_path)

		config.configure(env, conf_dir=os.path.join(env['builddir'], 'sconf_temp'))

		env.AppendUnique(LIBS=['streflop'])

		usropts.Save(usrcachefile, env)
		intopts.Save(intcachefile, env)

	# Substitute prefix in installprefix
	env['installprefix'] = env.subst(env['installprefix'])

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
		for d in filelist.list_directories(env, 'lua'):
			env.BuildDir(os.path.join(env['builddir'], d), d, duplicate = False)
		env.BuildDir(os.path.join(env['builddir'], 'tools/unitsync'), 'tools/unitsync', duplicate = False)
		for d in filelist.list_directories(env, 'AI'):
			env.BuildDir(os.path.join(env['builddir'], d), d, duplicate = False)

	#CacheDir support code
	if env.has_key('cachedir') and env['cachedir']:
		if not os.path.exists(env['cachedir']):
			os.makedirs(env['cachedir'])
		env.CacheDir(env['cachedir'])

	fix_windows_spawn(env)
