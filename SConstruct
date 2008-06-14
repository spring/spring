# Copyright (C) 2006  Tobi Vollebregt

# see rts/build/scons/*.py for the core of the build system

""" Available targets.
Each target has an equivalent install target. E.g. `CentralBuildAI' has
`install-CentralBuildAI' and the default target has `install'.

[default]

	spring
	unitsync
	GroupAI
		CentralBuildAI
		MetalMakerAI
		SimpleFormationAI
	GlobalAI
		JCAI
		NTAI
		OTAI
		TestGlobalAI
"""


import os, sys
sys.path.append('rts/build/scons')
import filelist

if sys.platform == 'win32':
	# force to mingw, otherwise picks up msvc
	env = Environment(tools = ['mingw', 'rts'], toolpath = ['.', 'rts/build/scons'])
else:
	env = Environment(tools = ['default', 'rts'], toolpath = ['.', 'rts/build/scons'])

spring_files = filelist.get_spring_source(env)

# spring.exe icon
if env['platform'] == 'windows':
	# resource builder (the default one of mingw gcc 4.1.1 crashes because of a popen bug in msvcrt.dll)
	if sys.platform == 'win32':
		rcbld = Builder(action = 'windres --use-temp-file -i $SOURCE -o $TARGET', suffix = '.o', src_suffix = '.rc')
	else:
		# dirty hack assuming on all non-win32 systems windres is called i586-mingw32msvc-windres..
		rcbld = Builder(action = 'i586-mingw32msvc-windres --use-temp-file -i $SOURCE -o $TARGET', suffix = '.o', src_suffix = '.rc')
	env.Append(BUILDERS = {'RES': rcbld})
	spring_files += env.RES(env['builddir'] + '/rts/build/scons/icon.rc', CPPPATH=[])

# calculate datadir locations
datadir = ['SPRING_DATADIR="\\"'+os.path.join(env['prefix'], env['datadir'])+'\\""',
           'SPRING_DATADIR_2="\\"'+os.path.join(env['prefix'], env['libdir'])+'\\""']

# Build UnixDataDirHandler.cpp separately from the other sources.  This is to prevent recompilation of
# the entire source if one wants to change just the install installprefix (and hence the datadir).

if env['platform'] != 'windows':
	ddlcpp = env.Object(os.path.join(env['builddir'], 'rts/System/Platform/Linux/DataDirLocater.cpp'), CPPDEFINES = env['CPPDEFINES']+env['spring_defines']+datadir)
	spring_files += [ddlcpp]
	spring = env.Program('game/spring', spring_files, CPPDEFINES=env['CPPDEFINES']+env['spring_defines'])
else: # create import library and .def file on Windows
	spring = env.Program('game/spring', spring_files, CPPDEFINES=env['CPPDEFINES']+env['spring_defines'], LINKFLAGS=env['LINKFLAGS'] + ['-Wl,--output-def,game/spring.def', '-Wl,--kill-at', '--add-stdcall-alias','-Wl,--out-implib,game/spring.a'] )

Alias('spring', spring)
Default(spring)
inst = env.Install(os.path.join(env['installprefix'], env['bindir']), spring)
Alias('install', inst)
Alias('install-spring', inst)

# Strip the executable if rts.py said so.
if env['strip']:
	env.AddPostAction(spring, Action([['strip','$TARGET']]))

# Build unitsync shared object
# HACK   we should probably compile libraries from 7zip, hpiutil2 and minizip
# so we don't need so much bloat here.
# Need a new env otherwise scons chokes on equal targets built with different flags.
uenv = env.Copy(builddir=os.path.join(env['builddir'], 'unitsync'))
uenv.AppendUnique(CPPDEFINES=['UNITSYNC', 'BITMAP_NO_OPENGL'])
for d in filelist.list_directories(uenv, 'rts'):
	uenv.BuildDir(os.path.join(uenv['builddir'], d), d, duplicate = False)
uenv.BuildDir(os.path.join(uenv['builddir'], 'tools/unitsync'), 'tools/unitsync', duplicate = False)
unitsync_files          = filelist.get_source(uenv, 'tools/unitsync', 'tools/unitsync/test/test.cpp');
unitsync_fs_files       = filelist.get_source(uenv, 'rts/System/FileSystem/');
unitsync_lua_files      = filelist.get_source(uenv, 'rts/lib/lua/src');
unitsync_7zip_files     = filelist.get_source(uenv, 'rts/lib/7zip');
unitsync_minizip_files  = filelist.get_source(uenv, 'rts/lib/minizip', 'rts/lib/minizip/iowin32.c');
unitsync_hpiutil2_files = filelist.get_source(uenv, 'rts/lib/hpiutil2');
unitsync_extra_files = [
	'rts/Game/GameVersion.cpp',
	'rts/Lua/LuaUtils.cpp',
	'rts/Lua/LuaParser.cpp',
	'rts/Map/MapParser.cpp',
	'rts/Rendering/Textures/Bitmap.cpp',
	'rts/Rendering/Textures/nv_dds.cpp',
	'rts/System/TdfParser.cpp',
	'rts/System/Platform/ConfigHandler.cpp',
	'rts/System/Platform/FileSystem.cpp',
]
for f in unitsync_fs_files:       unitsync_files += f
for f in unitsync_lua_files:      unitsync_files += f
for f in unitsync_7zip_files:     unitsync_files += f
for f in unitsync_minizip_files:  unitsync_files += f
for f in unitsync_hpiutil2_files: unitsync_files += f
for f in unitsync_extra_files:   unitsync_files += [os.path.join(uenv['builddir'], f)]

if env['platform'] == 'windows':
	# crosscompiles on buildbot need this, but native mingw builds fail
	# during linking
	if os.name != 'nt':
		unitsync_files.append('rts/lib/minizip/iowin32.c')
	for f in ['rts/System/Platform/Win/WinFileSystemHandler.cpp', 'rts/System/Platform/Win/RegHandler.cpp']:
		unitsync_files += [os.path.join(uenv['builddir'], f)]
	# Need the -Wl,--kill-at --add-stdcall-alias because TASClient expects undecorated stdcall functions.
	unitsync = uenv.SharedLibrary('game/unitsync', unitsync_files, LINKFLAGS=env['LINKFLAGS'] + ['-Wl,--kill-at', '--add-stdcall-alias'])
else:
	ddlcpp = uenv.SharedObject(os.path.join(uenv['builddir'], 'rts/System/Platform/Linux/DataDirLocater.cpp'), CPPDEFINES = uenv['CPPDEFINES']+datadir)
	unitsync_files += [ ddlcpp,
		os.path.join(uenv['builddir'], 'rts/System/Platform/Linux/DotfileHandler.cpp'),
		os.path.join(uenv['builddir'], 'rts/System/Platform/Linux/UnixFileSystemHandler.cpp')
	]
	unitsync = uenv.SharedLibrary('game/unitsync', unitsync_files)

Alias('unitsync', unitsync)
inst = env.Install(os.path.join(env['installprefix'], env['libdir']), unitsync)
Alias('install', inst)
Alias('install-unitsync', inst)

# Strip the DLL if rts.py said so.
if env['strip']:
	env.AddPostAction(unitsync, Action([['strip','$TARGET']]))

# Somehow unitsync fails to build with mingw:
#  "build\tools\unitsync\pybind.o(.text+0x129d): In function `initunitsync':
#   pybind.cpp:663: undefined reference to `_imp__Py_InitModule4TraceRefs'"
# Figured this out: this means we're attempting to build unitsync with debugging
# enabled against a python version with debugging disabled.
if env['platform'] != 'windows':
	Default(unitsync)

# Make a copy of the build environment for the AIs, but remove libraries and add include path.
# TODO: make separate SConstructs for AIs
aienv = env.Copy()
aienv.Append(CPPPATH = ['rts/ExternalAI'])

# Use subst() to substitute $installprefix in datadir.
install_dir = os.path.join(aienv['installprefix'], aienv['libdir'], 'AI/Helper-libs')

# store shared ai objects so newer scons versions don't choke with
# *** Two environments with different actions were specified for the same target
aiobjs = []
for f in filelist.get_shared_AI_source(aienv):
        while isinstance(f, list):
                f = f[0]
        fpath, fbase = os.path.split(f)
        fname, fext = fbase.rsplit('.', 1)
        aiobjs.append(aienv.SharedObject(os.path.join(fpath, fname + '-ai'), f))

#Build GroupAIs
for f in filelist.list_groupAIs(aienv, exclude_list=['build']):
	lib = aienv.SharedLibrary(os.path.join('game/AI/Helper-libs', f), aiobjs + filelist.get_groupAI_source(aienv, f))
	Alias(f, lib)         # Allow e.g. `scons CentralBuildAI' to compile just an AI.
	Alias('GroupAI', lib) # Allow `scons GroupAI' to compile all groupAIs.
	Default(lib)
	inst = env.Install(install_dir, lib)
	Alias('install', inst)
	Alias('install-GroupAI', inst)
	Alias('install-'+f, inst)
	if aienv['strip']:
		aienv.AddPostAction(lib, Action([['strip','$TARGET']]))

install_dir = os.path.join(aienv['installprefix'], aienv.subst(aienv['libdir']), 'AI/Bot-libs')

#Build GlobalAIs
for f in filelist.list_globalAIs(aienv, exclude_list=['build', 'CSAI', 'TestABICAI','AbicWrappersTestAI']):
	lib = aienv.SharedLibrary(os.path.join('game/AI/Bot-libs', f), aiobjs + filelist.get_globalAI_source(aienv, f))
	Alias(f, lib)          # Allow e.g. `scons JCAI' to compile just a global AI.
	Alias('GlobalAI', lib) # Allow `scons GlobalAI' to compile all globalAIs.
	Default(lib)
	inst = env.Install(install_dir, lib)
	Alias('install', inst)
	Alias('install-GlobalAI', inst)
	Alias('install-'+f, inst)
	if aienv['strip']:
		aienv.AddPostAction(lib, Action([['strip','$TARGET']]))

# build TestABICAI
# lib = aienv.SharedLibrary(os.path.join('game/AI/Bot-libs','TestABICAI'), ['game/spring.a'], CPPDEFINES = env# ['CPPDEFINES'] + ['BUILDING_AI'] )
# Alias('TestABICAI', lib)
# Alias('install-TestABICAI', inst)
# if sys.platform == 'win32':
# 	Alias('GlobalAI', lib)
# 	Default(lib)
# 	inst = env.Install(install_dir, lib)
# 	Alias('install', inst)
# 	Alias('install-GlobalAI', inst)
# 	if env['strip']:
# 		env.AddPostAction(lib, Action([['strip','$TARGET']]))

# Build streflop (which has it's own Makefile-based build system)
if not 'configure' in sys.argv and not 'test' in sys.argv and not 'install' in sys.argv:
	cmd = "CC=" + env['CC'] + " CXX=" + env['CXX'] + " --no-print-directory -C rts/lib/streflop"
	if env['fpmath'] == 'sse':
		cmd = "STREFLOP_SSE=1 " + cmd
	else:
		cmd = "STREFLOP_X87=1 " + cmd
	if env.GetOption('clean'):
		cmd += " clean"
	status = os.system("make " + cmd)
	if status != 0:
		# try with mingw32-make
		status = os.system("mingw32-make " + cmd)
	if status != 0:
		print "Failed building streflop!"
		env.Exit(1)
	else:
		print "Succes building streflop!"

# Use this to avoid an error message 'how to make target test ?'
env.Alias('test', None)

# Simple unit testing framework. In all 'Test' subdirectories, if a file 'test'
# exists, it is run. This test script should then compile the test(s) and run them.
if 'test' in sys.argv and env['platform'] != 'windows':
	for dir in filelist.list_directories(env, 'rts'):
		if dir.endswith('/Test'):
			test = os.path.join(dir, 'test')
			if os.path.isfile(test):
				os.system(test)


# Build gamedata zip archives

# Can't use these, we can't set the working directory and putting a SConscript
# in the respective directories doesn't work either because then the SConstript
# ends up in the zip too... Bah. SCons sucks. Just like autoshit and everything else btw.
#env.Zip('game/base/springcontent.sdz', filelist.list_files(env, 'installer/builddata/springcontent'))
#env.Zip('game/base/spring/bitmaps.sdz', filelist.list_files(env, 'installer/builddata/bitmaps'))

if not 'configure' in sys.argv and not 'test' in sys.argv and not 'install' in sys.argv:
	if sys.platform != 'win32':
		if env.GetOption('clean'):
			if os.system("rm -f game/base/springcontent.sdz"):
				env.Exit(1)
			if os.system("rm -f game/base/spring/bitmaps.sdz"):
				env.Exit(1)
			if os.system("rm -f game/base/maphelper.sdz"):
				env.Exit(1)
			if os.system("rm -f game/base/cursors.sdz"):
				env.Exit(1)
		else:
			if os.system("installer/make_gamedata_arch.sh"):
				env.Exit(1)

inst = env.Install(os.path.join(env['installprefix'], env['datadir'], 'base'), 'game/base/springcontent.sdz')
Alias('install', inst)
inst = env.Install(os.path.join(env['installprefix'], env['datadir'], 'base'), 'game/base/maphelper.sdz')
Alias('install', inst)
inst = env.Install(os.path.join(env['installprefix'], env['datadir'], 'base'), 'game/base/cursors.sdz')
Alias('install', inst)
inst = env.Install(os.path.join(env['installprefix'], env['datadir'], 'base/spring'), 'game/base/spring/bitmaps.sdz')
Alias('install', inst)

# install fonts
for font in os.listdir('game/fonts'):
	if not os.path.isdir(os.path.join('game/fonts', font)):
		inst = env.Install(os.path.join(env['installprefix'], env['datadir'], 'fonts'), os.path.join('game/fonts', font))
		Alias('install', inst)

# install startscripts
for f in os.listdir('game/startscripts'):
	if not os.path.isdir(f):
		inst = env.Install(os.path.join(env['installprefix'], env['datadir'], 'startscripts'), os.path.join('game/startscripts', f))
		Alias('install', inst)

# install some files from root of datadir
for f in ['cmdcolors.txt', 'ctrlpanel.txt', 'selectkeys.txt', 'uikeys.txt', 'teamcolors.lua']:
	inst = env.Install(os.path.join(env['installprefix'], env['datadir']), os.path.join('game', f))
	Alias('install', inst)

# install menu entry & icon
inst = env.Install(os.path.join(env['installprefix'], 'share/pixmaps'), 'rts/spring.png')
Alias('install', inst)
inst = env.Install(os.path.join(env['installprefix'], 'share/applications'), 'rts/spring.desktop')
Alias('install', inst)

# install AAI config files
aai_data=filelist.list_files_recursive(env, 'game/AI/AAI')
for f in aai_data:
	if not os.path.isdir(f):
		inst = env.Install(os.path.join(aienv['installprefix'], aienv['datadir'], os.path.dirname(f)[5:]), f)
		Alias('install', inst)

# install LuaUI files
for f in ['luaui.lua']:
	inst = env.Install(os.path.join(env['installprefix'], env['datadir']), os.path.join('game', f))
	Alias('install', inst)
luaui_files=filelist.list_files_recursive(env, 'game/LuaUI')
for f in luaui_files:
	if not os.path.isdir(f):
		inst = env.Install(os.path.join(env['installprefix'], env['datadir'], os.path.dirname(f)[5:]), f)
		Alias('install', inst)
