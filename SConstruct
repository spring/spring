# Copyright (C) 2006  Tobi Vollebregt

# see rts/build/scons/*.py for the core of the build system

""" Available targets.
Each target has an equivalent install target. E.g. `CentralBuildAI' has
`install-CentralBuildAI' and the default target has `install'.

[default]

	spring
	unitsync
	AIInterfaces
		C
		Java
	GroupAI
		CentralBuildAI
		MetalMakerAI
		SimpleFormationAI
	SkirmishAI
		RAI
		NTAI
		KAI
		KAIK
		AAI
		TestSkirmishAI
		NullAI
"""


import os, sys
sys.path.append('rts/build/scons')
import filelist

if sys.platform == 'win32':
	# force to mingw, otherwise picks up msvc
	env = Environment(tools = ['mingw', 'rts', 'gch'], toolpath = ['.', 'rts/build/scons'])
else:
	env = Environment(tools = ['default', 'rts',  'gch'], toolpath = ['.', 'rts/build/scons'])

if env['use_gch']:
	env['Gch'] = env.Gch('rts/System/StdAfx.h', CPPDEFINES=env['CPPDEFINES']+env['spring_defines'])[0]
else:
	import os.path
	if os.path.exists('rts/System/StdAfx.h.gch'):
		os.unlink('rts/System/StdAfx.h.gch')


################################################################################
### Build spring(.exe)
################################################################################
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


################################################################################
### Build unitsync shared object
################################################################################
# HACK   we should probably compile libraries from 7zip, hpiutil2 and minizip
# so we don't need so much bloat here.
# Need a new env otherwise scons chokes on equal targets built with different flags.
uenv = env.Copy(builddir=os.path.join(env['builddir'], 'unitsync'))
uenv.AppendUnique(CPPDEFINES=['UNITSYNC', 'BITMAP_NO_OPENGL'])

for d in filelist.list_directories(uenv, 'rts', exclude_list=["crashrpt"]):
	uenv.BuildDir(os.path.join(uenv['builddir'], d), d, duplicate = False)


uenv.BuildDir(os.path.join(uenv['builddir'], 'tools/unitsync'), 'tools/unitsync', duplicate = False)
unitsync_files          = filelist.get_source(uenv, 'tools/unitsync');
unitsync_fs_files       = filelist.get_source(uenv, 'rts/System/FileSystem/');
unitsync_lua_files      = filelist.get_source(uenv, 'rts/lib/lua/src');
unitsync_7zip_files     = filelist.get_source(uenv, 'rts/lib/7zip');
unitsync_minizip_files  = filelist.get_source(uenv, 'rts/lib/minizip', 'rts/lib/minizip/iowin32.c');
unitsync_hpiutil2_files = filelist.get_source(uenv, 'rts/lib/hpiutil2');
unitsync_extra_files = [
	'rts/Game/GameVersion.cpp',
	'rts/Lua/LuaUtils.cpp',
	'rts/Lua/LuaIO.cpp',
	'rts/Lua/LuaParser.cpp',
	'rts/Map/MapParser.cpp',
	'rts/Map/SMF/SmfMapFile.cpp',
	'rts/Rendering/Textures/Bitmap.cpp',
	'rts/Rendering/Textures/nv_dds.cpp',
	'rts/Sim/SideParser.cpp',
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
	for f in ['rts/System/Platform/Win/WinFileSystemHandler.cpp', 'rts/System/Platform/Win/DataDirLocater.cpp', 'rts/System/Platform/Win/RegHandler.cpp']:
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


################################################################################
### AIs
################################################################################
# Make a copy of the build environment for the AIs,
# TODO: make separate SConstructs for AIs
aienv = env.Copy()
aienv.Append(CPPPATH = ['rts/ExternalAI'])
aienv['LINKFLAGS'] += ['-Wl,--kill-at', '--add-stdcall-alias', '-mno-cygwin', '-lstdc++']
#print aienv['CPPDEFINES']

aiinterfaceenv = aienv.Copy()
aiinterfaceenv['CPPDEFINES'] += ['BUILDING_AI_INTERFACE']

aienv['CPPDEFINES'] += ['BUILDING_AI']

skirmishaienv = aienv.Copy()

groupaienv = aienv.Copy()


# stores shared objects so newer scons versions don't choke with
def create_shared_objects(env, fileList, suffix, additionalCPPDEFINES = []):
	objsList = []
	myEnv = env.Copy()
	myEnv['CPPDEFINES'] += additionalCPPDEFINES
	for f in fileList:
		while isinstance(f, list):
			f = f[0]
		fpath, fbase = os.path.split(f)
#		print "file: %s" % f
#		print "base: %s" % fbase
		fname, fext = fbase.rsplit('.', 1)
		objsList.append(myEnv.SharedObject(os.path.join(fpath, fname + suffix), f))
	return objsList

# retrieves the version of a Skirmish AI from the following file
# {spring_source}/AI/Skirmish/{ai_name}/VERSION
def fetch_ai_version(aiName, subDir = 'Skirmish'):
	version = 'VERSION_UNKNOWN'
	versionFile = os.path.join('AI', subDir, aiName, 'VERSION')
	if os.path.exists(versionFile):
		file = open(versionFile, 'r')
		version = file.readline().strip()
		file.close()
	return version

# appends the version to the end of the AI Interface name
def construct_aiinterface_libName(interfaceName):
	libName = interfaceName + '-' + fetch_ai_version(interfaceName, 'Interfaces')
	return libName

# appends the version to the end of the Skirmish AI name
def construct_skirmishai_libName(aiName):
	libName = aiName + '-' + fetch_ai_version(aiName, 'Skirmish')
	return libName

# appends the version to the end of the Group AI name
def construct_groupai_libName(aiName):
	libName = aiName + '-' + fetch_ai_version(aiName, 'Group')
	return libName

################################################################################
### Build AI Interface shared objects
################################################################################
install_aiinterfaces_lib_dir = os.path.join(aiinterfaceenv['installprefix'], aiinterfaceenv['libdir'], 'AI/Interfaces/impls')
install_aiinterfaces_data_dir = os.path.join(aiinterfaceenv['installprefix'], aiinterfaceenv['datadir'], 'AI/Interfaces/data')

# store shared ai-interface objects so newer scons versions don't choke with
# *** Two environments with different actions were specified for the same target
aiinterfaceobjs_main = create_shared_objects(aiinterfaceenv, filelist.get_shared_AIInterface_source(aiinterfaceenv), '-aiinterface')
aiinterfaceobjs_SharedLib = create_shared_objects(aiinterfaceenv, filelist.get_shared_AIInterface_source_SharedLib(aiinterfaceenv), '-aiinterface')

# Build
aiinterfaces_exclude_list=['build', 'Java']
aiinterfaces_needSharedLib_list=['C']
for baseName in filelist.list_AIInterfaces(aiinterfaceenv, exclude_list=aiinterfaces_exclude_list):
	libName = construct_aiinterface_libName(baseName) # eg. C-0.1
	print "AI Interface: " + libName
	objs = aiinterfaceobjs_main
	if baseName in aiinterfaces_needSharedLib_list:
		objs += aiinterfaceobjs_SharedLib
	mySource = objs + filelist.get_AIInterface_source(aiinterfaceenv, baseName)
	#lib = aiinterfaceenv.SharedLibrary(os.path.join('game/AI/Interfaces/impls', baseName), mySource)
	lib = aiinterfaceenv.SharedLibrary(os.path.join('game/AI/Interfaces/impls', libName), mySource)
	Alias(baseName, lib)       # Allow e.g. `scons Java' to compile just that specific AI interface.
	Alias('AIInterfaces', lib) # Allow `scons AIInterfaces' to compile all AI interfaces.
	Default(lib)
	inst = env.Install(install_aiinterfaces_lib_dir, lib)
	Alias('install', inst)
	Alias('install-AIInterfaces', inst)
	Alias('install-'+baseName, inst)
	if aiinterfaceenv['strip']:
		aiinterfaceenv.AddPostAction(lib, Action([['strip','$TARGET']]))

# install AI interface info files
aiinterfaces_data_dirs = filelist.list_directories(env, 'AI/Interfaces', exclude_list=aiinterfaces_exclude_list, recursively=False)
for f in aiinterfaces_data_dirs:
	baseName = os.path.basename(f)
	libName = construct_aiinterface_libName(baseName)
	#install_data_interface_dir = os.path.join(install_aiinterfaces_data_dir, baseName)
	install_data_interface_dir = os.path.join(install_aiinterfaces_data_dir, libName)
	infoFile = os.path.join(f, "InterfaceInfo.lua")
	if os.path.exists(infoFile):
		inst = env.Install(install_data_interface_dir, infoFile)
		Alias('install', inst)
		Alias('install-AIInterfaces', inst)
		Alias('install-'+baseName, inst)

################################################################################
### Build Skirmish AI shared objects
################################################################################
install_skirmishai_lib_dir = os.path.join(skirmishaienv['installprefix'], skirmishaienv['libdir'], 'AI/Skirmish/impls')
install_skirmishai_data_dir = os.path.join(skirmishaienv['installprefix'], skirmishaienv['datadir'], 'AI/Skirmish/data')

# store shared ai objects so newer scons versions don't choke with
# *** Two environments with different actions were specified for the same target
skirmishaiobjs_main = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source(skirmishaienv), '-skirmishai')
skirmishaiobjs_mainCregged = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source(skirmishaienv), '-skirmishai_creg', ['USING_CREG'])
skirmishaiobjs_creg = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source_Creg(skirmishaienv), '-skirmishai_creg', ['USING_CREG'])
skirmishaiobjs_LegacyCpp = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source_LegacyCpp(skirmishaienv), '-skirmishai')
skirmishaiobjs_LegacyCppCregged = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source_LegacyCpp(skirmishaienv), '-skirmishai_creg', ['USING_CREG'])

# Build
skirmishai_exclude_list=['build', 'CSAI', 'TestABICAI','AbicWrappersTestAI']
skirmishai_isLegacyCpp_list=['AAI', 'KAIK-0.13', 'RAI-0.553', 'NullLegacyCppAI', 'KAI-0.2', 'NTai']
skirmishai_needCreg_list=['KAIK-0.13', 'KAI-0.2']
for baseName in filelist.list_skirmishAIs(skirmishaienv, exclude_list=skirmishai_exclude_list):
	libName = construct_skirmishai_libName(baseName) # eg. RAI-0.600
	print "Skirmish AI: " + libName
	useCreg = baseName in skirmishai_needCreg_list
	isLegacyCpp = baseName in skirmishai_isLegacyCpp_list
	myEnv = skirmishaienv.Copy()
	if useCreg:
		myEnv['CPPDEFINES'] += ['USING_CREG']
	objs = []
	if useCreg:
		objs += skirmishaiobjs_mainCregged
		objs += skirmishaiobjs_creg
	else:
		objs += skirmishaiobjs_main
	if isLegacyCpp:
		if useCreg:
			objs += skirmishaiobjs_LegacyCppCregged
		else:
			objs += skirmishaiobjs_LegacyCpp
	mySource = objs + filelist.get_skirmishAI_source(myEnv, baseName)
	lib = myEnv.SharedLibrary(os.path.join('game/AI/Skirmish/impls', baseName), mySource)
	#lib = myEnv.SharedLibrary(os.path.join('game/AI/Skirmish/impls', libName), mySource)
	Alias(baseName, lib)            # Allow e.g. `scons JCAI' to compile just a skirmish AI.
	Alias('SkirmishAI', lib) # Allow `scons SkirmishAI' to compile all skirmishAIs.
	Default(lib)
	inst = env.Install(install_skirmishai_lib_dir, lib)
	Alias('install', inst)
	Alias('install-SkirmishAI', inst)
	Alias('install-'+baseName, inst)
	if myEnv['strip']:
		myEnv.AddPostAction(lib, Action([['strip','$TARGET']]))

# install Skirmish AI info and options files
skirmishai_data_dirs=filelist.list_directories(env, 'AI/Skirmish', exclude_list=skirmishai_exclude_list, recursively=False)
for f in skirmishai_data_dirs:
	baseName = os.path.basename(f)
	libName = construct_aiinterface_libName(baseName)
	install_data_ai_dir = os.path.join(install_skirmishai_data_dir, baseName)
	#install_data_ai_dir = os.path.join(install_skirmishai_data_dir, libName)
	infoFile = os.path.join(f, "AIInfo.lua")
	if os.path.exists(infoFile):
		inst = env.Install(install_data_ai_dir, infoFile)
		Alias('install', inst)
		Alias('install-SkirmishAI', inst)
		Alias('install-'+baseName, inst)
	optionsFile = os.path.join(f, "AIOptions.lua")
	if os.path.exists(optionsFile):
		inst = env.Install(install_data_ai_dir, optionsFile)
		Alias('install', inst)
		Alias('install-SkirmishAI', inst)
		Alias('install-'+baseName, inst)

# install AAI config files
aai_data=filelist.list_files_recursive(env, 'game/AI/AAI')
for f in aai_data:
	if not os.path.isdir(f):
		inst = env.Install(os.path.join(skirmishaienv['installprefix'], skirmishaienv['datadir'], os.path.dirname(f)[5:]), f)
		Alias('install', inst)

################################################################################
### Build Group AI shared objects
################################################################################
install_groupai_lib_dir = os.path.join(groupaienv['installprefix'], groupaienv['libdir'], 'AI/Helper-libs')
#install_groupai_data_dir = os.path.join(groupaienv['installprefix'], groupaienv['datadir'], 'AI/Helper-libs/data')

# store shared ai objects so newer scons versions don't choke with
# *** Two environments with different actions were specified for the same target
groupaiobjs_main = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source(groupaienv), '-groupai')
groupaiobjs_mainCregged = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source(groupaienv), '-groupai_creg', ['USING_CREG'])
groupaiobjs_creg = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source_Creg(groupaienv), '-groupai_creg', ['USING_CREG'])
groupaiobjs_LegacyCpp = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source_LegacyCpp(groupaienv), '-groupai')
groupaiobjs_LegacyCppCregged = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source_LegacyCpp(groupaienv), '-groupai_creg', ['USING_CREG'])

# Build
groupai_exclude_list=['build']
groupai_isLegacyCpp_list=['?', '??']
groupai_needCreg_list=[]
for baseName in filelist.list_groupAIs(groupaienv, exclude_list=groupai_exclude_list):
	libName = construct_groupai_libName(baseName) # eg. MetalMaker-1.0
	print "Group AI: " + libName
	#TODO remove the True in the next line, uncomment the rest, and actualize groupai_needCreg_list
	useCreg = True #baseName in groupai_needCreg_list
	#TODO remove the True in the next line, uncomment the rest, and actualize groupai_isLegacyCpp_list
	isLegacyCpp = True #baseName in groupai_isLegacyCpp_list
	myEnv = groupaienv.Copy()
	if useCreg:
		myEnv['CPPDEFINES'] += ['USING_CREG']
	objs = []
	if useCreg:
		objs += groupaiobjs_mainCregged
		objs += groupaiobjs_creg
	else:
		objs += groupaiobjs_main
	if isLegacyCpp:
		if useCreg:
			objs += groupaiobjs_LegacyCppCregged
		else:
			objs += groupaiobjs_LegacyCpp
	mySource = objs + filelist.get_groupAI_source(myEnv, baseName)
	lib = myEnv.SharedLibrary(os.path.join('game/AI/Helper-libs', baseName), mySource)
	#lib = myEnv.SharedLibrary(os.path.join('game/AI/Helper-libs', libName), mySource)
	Alias(baseName, lib)         # Allow e.g. `scons CentralBuildAI' to compile just an AI.
	Alias('GroupAI', lib) # Allow `scons GroupAI' to compile all groupAIs.
	Default(lib)
	inst = env.Install(install_groupai_lib_dir, lib)
	Alias('install', inst)
	Alias('install-GroupAI', inst)
	Alias('install-'+baseName, inst)
	if myEnv['strip']:
		myEnv.AddPostAction(lib, Action([['strip','$TARGET']]))


################################################################################
### Build streflop (which has it's own Makefile-based build system)
if not 'configure' in sys.argv and not 'test' in sys.argv and not 'install' in sys.argv:
	cmd = "CC=" + env['CC'] + " CXX=" + env['CXX'] + " --no-print-directory -C rts/lib/streflop"
	if env.has_key('streflop_extra'):
		cmd += " " + env['streflop_extra']
	if env['fpmath'] == 'sse':
		cmd = "STREFLOP_SSE=1 " + cmd
	else:
		cmd = "STREFLOP_X87=1 " + cmd
	if env['platform'] == 'windows':
		cmd += " WIN32=1"
	if env.GetOption('clean'):
		cmd += " clean"
	print 'streflop options:', cmd
	if env['platform'] == 'freebsd':
		status = os.system("gmake " + cmd)
	else:
		status = os.system("make " + cmd)
	if status != 0:
		# try with mingw32-make
		status = os.system("mingw32-make " + cmd)
	if status != 0:
		print "Failed building streflop!"
		env.Exit(1)
	else:
		print "Success building streflop!"

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


################################################################################
# Build gamedata zip archives
################################################################################
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
inst = env.Install(os.path.join(env['installprefix'], 'share/applications'), 'installer/freedesktop/applications/spring.desktop')
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
