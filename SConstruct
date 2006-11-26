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
    env = Environment(tools = ['mingw', 'rts'], toolpath = ['.', 'rts/build/scons']) # force to mingw, otherwise picks up msvc
else:
    env = Environment(tools = ['default', 'rts'], toolpath = ['.', 'rts/build/scons'])

spring_files = filelist.get_spring_source(env)

# Build UnixDataDirHandler.cpp separately from the other sources.  This is to prevent recompilation of
# the entire source if one wants to change just the install prefix (and hence the datadir).

if env['platform'] != 'windows':
	ufshcpp = env.Object(os.path.join(env['builddir'], 'rts/System/Platform/Linux/UnixFileSystemHandler.cpp'), CPPDEFINES = env['CPPDEFINES']+env['spring_defines']+['SPRING_DATADIR="\\"'+env['datadir']+'\\""'])
	spring_files += [ufshcpp]
	spring = env.Program('game/spring', spring_files, CPPDEFINES=env['CPPDEFINES']+env['spring_defines'])
else: # create import library and .def file on Windows
    spring = env.Program('game/spring', spring_files, CPPDEFINES=env['CPPDEFINES']+env['spring_defines'], LINKFLAGS=env['LINKFLAGS'] + ['-Wl,--output-def,game/spring.def', '-Wl,--kill-at', '--add-stdcall-alias','-Wl,--out-implib,game/spring.a'] )    

Alias('spring', spring)
Default(spring)
inst = Install(os.path.join(env['prefix'], 'bin'), spring)
Alias('install', inst)
Alias('install-spring', inst)

# Strip the executable if rts.py said so.
if env['strip']:
	env.AddPostAction(spring, Action([['strip','$TARGET']]))

# Build unitsync shared object
# HACK   we should probably compile libraries from 7zip, hpiutil2 and minizip
# so we don't need so much bloat here.
unitsync_files = filelist.get_source(env, 'tools/unitsync') + \
	['rts/Rendering/Textures/Bitmap.cpp',
	'rts/Rendering/Textures/nv_dds.cpp',
	'rts/System/TdfParser.cpp',
	'rts/System/FileSystem/Archive7Zip.cpp',
	'rts/System/FileSystem/ArchiveBuffered.cpp',
	'rts/System/FileSystem/ArchiveDir.cpp',
	'rts/System/FileSystem/ArchiveFactory.cpp',
	'rts/System/FileSystem/ArchiveHPI.cpp',
	'rts/System/FileSystem/ArchiveScanner.cpp',
	'rts/System/FileSystem/ArchiveZip.cpp',
	'rts/System/FileSystem/FileHandler.cpp',
	'rts/System/FileSystem/VFSHandler.cpp',
	'rts/System/Platform/ConfigHandler.cpp',
	'rts/System/Platform/FileSystem.cpp',
	'rts/lib/7zip/7zAlloc.c',
	'rts/lib/7zip/7zBuffer.c',
	'rts/lib/7zip/7zCrc.c',
	'rts/lib/7zip/7zDecode.c',
	'rts/lib/7zip/7zExtract.c',
	'rts/lib/7zip/7zHeader.c',
	'rts/lib/7zip/7zIn.c',
	'rts/lib/7zip/7zItem.c',
	'rts/lib/7zip/7zMethodID.c',
	'rts/lib/7zip/LzmaDecode.c',
	'rts/lib/hpiutil2/hpientry.cpp',
	'rts/lib/hpiutil2/hpifile.cpp',
	'rts/lib/hpiutil2/hpiutil.cpp',
	'rts/lib/hpiutil2/scrambledfile.cpp',
	'rts/lib/hpiutil2/sqshstream.cpp',
	'rts/lib/hpiutil2/substream.cpp',
	'rts/lib/minizip/ioapi.c',
	'rts/lib/minizip/unzip.c',
	'rts/lib/minizip/zip.c']

if env['platform'] == 'windows':
	unitsync_files += ['rts/lib/minizip/iowin32.c', 'rts/System/Platform/Win/WinFileSystemHandler.cpp', 'rts/System/Platform/Win/RegHandler.cpp']
else:
	ufshcpp = env.SharedObject(os.path.join(env['builddir'], 'rts/System/Platform/Linux/UnixFileSystemHandler.cpp'), CPPDEFINES = env['CPPDEFINES']+['SPRING_DATADIR="\\"'+env['datadir']+'\\""'])
	unitsync_files += ['rts/System/Platform/Linux/DotfileHandler.cpp', ufshcpp]

unitsync = env.SharedLibrary('omni/unitsync', unitsync_files)
unitsync2 = env.SharedLibrary('UnityLobby/client/unitsync', unitsync_files)
Alias('unitsync', unitsync, unitsync2)

# Somehow unitsync fails to build with mingw:
#  "build\tools\unitsync\pybind.o(.text+0x129d): In function `initunitsync':
#   pybind.cpp:663: undefined reference to `_imp__Py_InitModule4TraceRefs'"
if env['platform'] != 'windows':
	Default(unitsync, unitsync2)

# Make a copy of the build environment for the AIs, but remove libraries and add include path.
aienv = env.Copy(LIBS=[], LIBPATH=[])
aienv.Append(CPPPATH = ['rts/ExternalAI'])

# Use subst() to substitute $prefix in datadir.
install_dir = os.path.join(aienv.subst(aienv['datadir']), 'AI/Helper-libs')

#Build GroupAIs
for f in filelist.list_groupAIs(aienv, exclude_list=['build']):
	lib = aienv.SharedLibrary(os.path.join('game/AI/Helper-libs', f), filelist.get_groupAI_source(aienv, f))
	Alias(f, lib)         # Allow e.g. `scons CentralBuildAI' to compile just an AI.
	Alias('GroupAI', lib) # Allow `scons GroupAI' to compile all groupAIs.
	Default(lib)
	inst = Install(install_dir, lib)
	Alias('install', inst)
	Alias('install-GroupAI', inst)
	Alias('install-'+f, inst)

install_dir = os.path.join(aienv.subst(aienv['datadir']), 'AI/Bot-libs')

#Build GlobalAIs
for f in filelist.list_globalAIs(aienv, exclude_list=['build', 'CSAI', 'TestABICAI','AbicWrappersTestAI']):
	lib = aienv.SharedLibrary(os.path.join('game/AI/Bot-libs', f), filelist.get_globalAI_source(aienv, f))
	Alias(f, lib)          # Allow e.g. `scons JCAI' to compile just a global AI.
	Alias('GlobalAI', lib) # Allow `scons GlobalAI' to compile all globalAIs.
	Default(lib)
	inst = Install(install_dir, lib)
	Alias('install', inst)
	Alias('install-GlobalAI', inst)
	Alias('install-'+f, inst)

# build TestABICAI
lib = aienv.SharedLibrary(os.path.join('game/AI/Bot-libs','TestABICAI'), ['AI/Global/TestABICAI/myaimingw.cpp','game/spring.a'], CPPDEFINES = env['CPPDEFINES'] + ['BUILDING_AI'] )
Alias('TestABICAI', lib)
Alias('install-TestABICAI', inst)
if sys.platform == 'win32':
	Alias('GlobalAI', lib)
	Default(lib)
	inst = Install(install_dir, lib)
	Alias('install', inst)
	Alias('install-GlobalAI', inst)

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
	if status:
		print "Failed building streflop!"
		env.Exit(status)
	else:
		print "Succes building streflop!"

# Use this to avoid an error message 'how to make target test ?'
# This can be replaced for unit testing code at any time (in other branch for example).
env.Alias('test', None)

# Build gamedata zip archives

# Can't use these, we can't set the working directory and putting a SConscript
# in the respective directories doesn't work either because then the SConstript
# ends up in the zip too... Bah. SCons sucks. Just like autoshit and everything else btw.
#env.Zip('game/base/springcontent.sdz', filelist.list_files(env, 'installer/builddata/springcontent'))
#env.Zip('game/base/spring/bitmaps.sdz', filelist.list_files(env, 'installer/builddata/bitmaps'))

if not 'configure' in sys.argv and not 'test' in sys.argv and not 'install' in sys.argv:
	if sys.platform != 'win32':
		if env.GetOption('clean'):
			os.system("rm -f game/base/springcontent.sdz")
			os.system("rm -f game/base/spring/bitmaps.sdz")
		else:
			os.system("installer/make_gamedata_arch.sh")

inst = Install(os.path.join(env.subst(env['datadir']), 'base'), 'game/base/springcontent.sdz')
Alias('install', inst)
inst = Install(os.path.join(env.subst(env['datadir']), 'base/spring'), 'game/base/spring/bitmaps.sdz')
Alias('install', inst)

# install shaders
shaders=os.listdir('game/shaders')
for shader in shaders:
	if not os.path.isdir(os.path.join('game/shaders', shader)):
		inst = Install(os.path.join(env.subst(env['datadir']), 'shaders'), os.path.join('game/shaders', shader))
		Alias('install', inst)

# install menu entry & icon
inst = Install(os.path.join(env['prefix'], 'share/pixmaps'), 'rts/spring.png')
Alias('install', inst)
inst = Install(os.path.join(env['prefix'], 'share/applications'), 'rts/spring.desktop')
Alias('install', inst)
