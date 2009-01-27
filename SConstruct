# Copyright (C) 2006  Tobi Vollebregt

# see rts/build/scons/*.py for the core of the build system

""" Available targets.
Each target has an equivalent install target. E.g. `NullAI' has
`install-NullAI' and the default target has `install'.

[default]

	spring
	unitsync
	AIInterfaces
		C
		Java
	SkirmishAI
		RAI
		NTAI
		KAI
		KAIK
		AAI
		JCAI
		NullAI
		NullLegacyCppAI
		NullJavaAI
		NullOOJavaAI
"""


import os, sys
sys.path.append('rts/build/scons')
import filelist

if sys.platform == 'win32':
	# force to mingw, otherwise picks up msvc
	myTools = ['mingw', 'rts', 'gch']
else:
	myTools = ['default', 'rts', 'gch']

env = Environment(tools = myTools, toolpath = ['.', 'rts/build/scons'])
#env = Environment(tools = myTools, toolpath = ['.', 'rts/build/scons'], ENV = {'PATH' : os.environ['PATH']})

if env['use_gch']:
	env['Gch'] = env.Gch('rts/System/StdAfx.h', CPPDEFINES=env['CPPDEFINES']+env['spring_defines'])[0]
else:
	import os.path
	if os.path.exists('rts/System/StdAfx.h.gch'):
		os.unlink('rts/System/StdAfx.h.gch')

################################################################################
### Build streflop (which has it's own Makefile-based build system)
################################################################################

# stores shared objects so newer scons versions don't choke with
def create_static_objects(env, fileList, suffix, additionalCPPDEFINES = []):
	objsList = []
	myEnv = env.Clone()
	myEnv.AppendUnique(CPPDEFINES = additionalCPPDEFINES)
	for f in fileList:
		while isinstance(f, list):
			f = f[0]
		fpath, fbase = os.path.split(f)
		fname, fext = fbase.rsplit('.', 1)
		objsList.append(myEnv.StaticObject(os.path.join(fpath, fname + suffix), f))
	return objsList

# setup build environment
senv = env.Clone(builddir=os.path.join(env['builddir'], 'streflop'))
senv['CPPPATH'] = []
senv.BuildDir(os.path.join(senv['builddir'], 'rts/lib/streflop'), 'rts/lib/streflop', duplicate = False)
for d in filelist.list_directories(senv, 'rts/lib/streflop', exclude_list=[]):
	senv.BuildDir(os.path.join(senv['builddir'], d), d, duplicate = False)

# setup flags and defines
if senv['fpmath'] == 'sse':
	senv['CPPDEFINES'] = ['STREFLOP_SSE=1']
else:
	senv['CPPDEFINES'] = ['STREFLOP_X87=1']
senv['CXXFLAGS'] = ['-Wall', '-O3', '-pipe', '-g', '-mno-tls-direct-seg-refs', '-fsingle-precision-constant', '-frounding-math', '-fsignaling-nans', '-fno-strict-aliasing', '-mieee-fp']
senv.AppendUnique(CXXFLAGS = senv['streflop_extra'])

# create the libm sub-part build environment
slibmenv = senv.Clone()
slibmenv.AppendUnique(CPPPATH = ['rts/lib/streflop/libm/headers'])

# gather the sources
sobjs_flt32 = create_static_objects(slibmenv, filelist.get_source(slibmenv, 'rts/lib/streflop/libm/flt-32'), '-streflop-flt32', ['LIBM_COMPILING_FLT32'])
#sobjs_dbl64 = create_static_objects(slibmenv, filelist.get_source(slibmenv, 'rts/lib/streflop/libm/dbl-64'), '-streflop-dbl64', ['LIBM_COMPILING_DBL64'])
#sobjs_ldbl96 = create_static_objects(slibmenv, filelist.get_source(slibmenv, 'rts/lib/streflop/libm/ldbl-96'), '-streflop-ldbl96', ['LIBM_COMPILING_LDBL96'])
streflopSource_tmp = [
		'rts/lib/streflop/SMath.cpp',
		'rts/lib/streflop/Random.cpp'
		]
streflopSource = []
for f in streflopSource_tmp:   streflopSource += [os.path.join(senv['builddir'], f)]
streflopSource += sobjs_flt32
# not needed => safer (and faster) to not compile them at all
#streflopSource += sobjs_flt32 + sobjs_dbl64 + sobjs_ldbl96

# compile
streflop_lib = senv.StaticLibrary(senv['builddir'], streflopSource)
env['streflop_lib'] = streflop_lib
Alias('streflop', streflop_lib) # Allow `scons streflop' to compile just streflop

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
if env['platform'] == 'windows':
	datadir = []
else:
	datadir = ['SPRING_DATADIR="\\"'+os.path.join(env['prefix'], env['datadir'])+'\\""',
			'SPRING_DATADIR_2="\\"'+os.path.join(env['prefix'], env['libdir'])+'\\""']

# Build DataDirLocater.cpp separately from the other sources.  This is to prevent recompilation of
# the entire source if one wants to change just the install installprefix (and hence the datadir).

ddlcpp = env.Object(os.path.join(env['builddir'], 'rts/System/FileSystem/DataDirLocater.cpp'), CPPDEFINES = env['CPPDEFINES']+env['spring_defines']+datadir)
spring_files += [ddlcpp]
spring_files += [streflop_lib]
if env['platform'] != 'windows':
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
usync_builddir = os.path.join(env['builddir'], 'unitsync')
uenv = env.Clone(builddir=usync_builddir)
uenv.AppendUnique(CPPDEFINES=['UNITSYNC', 'BITMAP_NO_OPENGL'])

def remove_precompiled_header(env):
	while 'USE_PRECOMPILED_HEADER' in env['CPPDEFINES']:
		env['CPPDEFINES'].remove('USE_PRECOMPILED_HEADER')
	while '-DUSE_PRECOMPILED_HEADER' in env['CFLAGS']:
		env['CFLAGS'].remove('-DUSE_PRECOMPILED_HEADER')
	while '-DUSE_PRECOMPILED_HEADER' in env['CXXFLAGS']:
		env['CXXFLAGS'].remove('-DUSE_PRECOMPILED_HEADER')

remove_precompiled_header(uenv)

def usync_get_source(*args, **kwargs):
        return filelist.get_source(uenv, ignore_builddir=True, *args, **kwargs)

uenv.BuildDir(os.path.join(uenv['builddir'], 'tools/unitsync'), 'tools/unitsync', duplicate = False)
unitsync_files          = usync_get_source('tools/unitsync')
unitsync_fs_files       = usync_get_source('rts/System/FileSystem/', exclude_list=('rts/System/FileSystem/DataDirLocater.cpp'));
unitsync_lua_files      = usync_get_source('rts/lib/lua/src');
unitsync_7zip_files     = usync_get_source('rts/lib/7zip');
unitsync_minizip_files  = usync_get_source('rts/lib/minizip', 'rts/lib/minizip/iowin32.c');
unitsync_hpiutil2_files = usync_get_source('rts/lib/hpiutil2');
unitsync_extra_files = [
	'rts/Game/GameVersion.cpp',
	'rts/Lua/LuaUtils.cpp',
	'rts/Lua/LuaIO.cpp',
	'rts/Lua/LuaParser.cpp',
	'rts/Map/MapParser.cpp',
	'rts/Map/SMF/SmfMapFile.cpp',
	'rts/Rendering/Textures/Bitmap.cpp',
	'rts/Rendering/Textures/nv_dds.cpp',
	'rts/Sim/Misc/SideParser.cpp',
	'rts/System/Info.cpp',
	'rts/System/Option.cpp',
	'rts/System/ConfigHandler.cpp',
	'rts/System/LogOutput.cpp',
]
unitsync_files.extend(unitsync_fs_files)
unitsync_files.extend(unitsync_lua_files)
unitsync_files.extend(unitsync_7zip_files)
unitsync_files.extend(unitsync_minizip_files)
unitsync_files.extend(unitsync_hpiutil2_files)
unitsync_files.extend(unitsync_extra_files)

if env['platform'] == 'windows':
	# crosscompiles on buildbot need this, but native mingw builds fail
	# during linking
	if os.name != 'nt':
		unitsync_files.append('rts/lib/minizip/iowin32.c')
	unitsync_files.append('rts/System/FileSystem/DataDirLocater.cpp')
        # some scons stupidity
        unitsync_objects = [uenv.SharedObject(source=f, target=os.path.join(uenv['builddir'], f)+'.o') for f in unitsync_files]
	# Need the -Wl,--kill-at --add-stdcall-alias because TASClient expects undecorated stdcall functions.
	unitsync = uenv.SharedLibrary('game/unitsync', unitsync_objects, LINKFLAGS=env['LINKFLAGS'] + ['-Wl,--kill-at', '--add-stdcall-alias'])
else:
	ddlcpp = uenv.SharedObject(os.path.join(env['builddir'], 'rts/System/FileSystem/DataDirLocater.cpp'), CPPDEFINES = uenv['CPPDEFINES']+datadir)
        # some scons stupidity
        unitsync_objects = [uenv.SharedObject(source=f, target=os.path.join(uenv['builddir'], f)+'.os') for f in unitsync_files]
	unitsync_objects += [ ddlcpp ]
	unitsync = uenv.SharedLibrary('game/unitsync', unitsync_objects)

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


#################################################################################
#### AIs
#################################################################################
## Make a copy of the build environment for the AIs,
## but remove libraries and add include path.
## TODO: make separate SConstructs for AIs
#aienv = env.Clone()
#aienv.AppendUnique(CPPPATH = ['rts/ExternalAI'])
#aienv.AppendUnique(LINKFLAGS = ['-Wl,--kill-at', '--add-stdcall-alias', '-mno-cygwin', '-lstdc++'])
##print aienv['CPPDEFINES']
#
#aiinterfaceenv = aienv.Clone()
#aiinterfaceenv.AppendUnique(CPPDEFINES = ['BUILDING_AI_INTERFACE'])
#
#aienv.AppendUnique(CPPDEFINES = ['BUILDING_AI'])
#
#skirmishaienv = aienv.Clone()
#
##groupaienv = aienv.Clone()
#
## stores shared objects so newer scons versions don't choke with
#def create_shared_objects(env, fileList, suffix, additionalCPPDEFINES = []):
#	objsList = []
#	myEnv = env.Clone()
#	myEnv.AppendUnique(CPPDEFINES = additionalCPPDEFINES)
#	for f in fileList:
#		while isinstance(f, list):
#			f = f[0]
#		fpath, fbase = os.path.split(f)
##		print "file: %s" % f
##		print "base: %s" % fbase
#		fname, fext = fbase.rsplit('.', 1)
#		objsList.append(myEnv.SharedObject(os.path.join(fpath, fname + suffix), f))
#	return objsList
#
## retrieves the version of a Skirmish AI from the following file
## {spring_source}/AI/Skirmish/{ai_name}/VERSION
#def fetch_ai_version(aiName, subDir = 'Skirmish'):
#	version = 'VERSION_UNKNOWN'
#	versionFile = os.path.join('AI', subDir, aiName, 'VERSION')
#	if os.path.exists(versionFile):
#		file = open(versionFile, 'r')
#		version = file.readline().strip()
#		file.close()
#	return version
#
## appends the version to the end of the AI Interface name
#def construct_aiinterface_libName(interfaceName):
#	libName = interfaceName + '-' + fetch_ai_version(interfaceName, 'Interfaces')
#	return libName
#
#def getLocalShellExecPostfix():
#	#print('sys.platform: ' + sys.platform)
#	if sys.platform == 'win32' or sys.platform == 'win64':
#		postfix = 'bat'
#	else:
#		postfix = 'sh'
#	return postfix
#
#def createJavaClasspath(path):
#	# helper function to get a list of all subdirectories
#	def addJars(jarList, dirname, names):
#		# internal function to pass to os.path.walk
#		for n in names:
#			f = os.path.join(dirname, n)
#			if os.path.isfile(f) and os.path.splitext(f)[1] == ".jar":
#				jarList.append(f)
#				#print('found jar: ' + n)
#
#	jarList = []
#	os.path.walk(path, addJars, jarList)
#	clsPath = path
#	for j in jarList:
#		clsPath = clsPath + os.pathsep + j
#	#print('clsPath: ' + clsPath)
#	return clsPath
#
## instals files plus empty directories recursively, preserving directory structure
#def installDataDir(env, dstPath, srcPath, instList):
#	if os.path.exists(srcPath):
#		files = filelist.list_files_recursive(env, srcPath, exclude_regexp = '\.svn', exclude_dirs = False, path_relative = True)
#		for f in files:
#			f_src_file = os.path.join(srcPath, f)
#			f_dst_path = os.path.join(dstPath, os.path.split(f)[0])
#			f_dst_file = os.path.join(dstPath, f)
#			if not (os.path.isdir(f_src_file) and os.path.exists(f_dst_file)):
#				instList += [env.Install(f_dst_path, f_src_file)]
#
#################################################################################
#### Build AI Interface shared objects
#################################################################################
#install_aiinterfaces_dir = os.path.join(aiinterfaceenv['installprefix'], aiinterfaceenv['datadir'], 'AI/Interfaces')
#
## store shared ai-interface objects so newer scons versions don't choke with
## *** Two environments with different actions were specified for the same target
#aiinterfaceobjs_main = create_shared_objects(aiinterfaceenv, filelist.get_shared_AIInterface_source(aiinterfaceenv), '-aiinterface')
#aiinterfaceobjs_SharedLib = create_shared_objects(aiinterfaceenv, filelist.get_shared_AIInterface_source_SharedLib(aiinterfaceenv), '-aiinterface')
#
## Build
#aiinterfaces_exclude_list=['build']
#aiinterfaces_needSharedLib_list=['C']
#aiinterfaces_needStreflop_list=['Java']
#javaInterfaceCP = ''
#javaInterfaceJar = ''
#for baseName in filelist.list_AIInterfaces(aiinterfaceenv, exclude_list=aiinterfaces_exclude_list):
#	aiInterfaceVersion = fetch_ai_version(baseName, 'Interfaces')
#	print "Found AI Interface: " + baseName + " " + aiInterfaceVersion
#	myEnv = aiinterfaceenv.Clone()
#	install_data_interface_dir = os.path.join(install_aiinterfaces_dir, baseName, aiInterfaceVersion)
#	instList = []
#	objs = aiinterfaceobjs_main
#	if baseName in aiinterfaces_needSharedLib_list:
#		objs += aiinterfaceobjs_SharedLib
#	if baseName in aiinterfaces_needStreflop_list:
#		if env['fpmath'] == 'sse':
#			myEnv.AppendUnique(CPPDEFINES=['STREFLOP_SSE'])
#		else:
#			myEnv.AppendUnique(CPPDEFINES=['STREFLOP_X87'])
#		myEnv.AppendUnique(CXXFLAGS = ['-Irts/lib/streflop'])
#		myEnv.AppendUnique(LIBS = ['streflop'])
#	mySource = objs + filelist.get_AIInterface_source(myEnv, baseName)
#	if baseName == 'Java':
#		myEnv.AppendUnique(LIBS = ['jvm'])
#		# generate class files
#		javaWrapperScript = 'java_generateWrappers.' + getLocalShellExecPostfix()
#		javaWrapperScriptPath = os.path.join('AI/Interfaces', baseName, 'bin')
#		if sys.platform == 'win32':
#			javaWrapperCmd = javaWrapperScript
#		else:
#			javaWrapperCmd = './' + javaWrapperScript
#		javaWrapperBld = myEnv.Builder(action = javaWrapperCmd, chdir = javaWrapperScriptPath)
#		myEnv.Append(BUILDERS = {'JavaWrapper' : javaWrapperBld})
#		javaSrcGen = myEnv.JavaWrapper(source = mySource)
#		Alias(baseName, javaSrcGen)
#		Alias('AIInterfaces', javaSrcGen)
#		Default(javaSrcGen)
#		# compile the Java part
#		javaSrc = os.path.join('AI/Interfaces', baseName, 'java/src')
#		javaClasses = os.path.join(myEnv['builddir'], 'AI/Interfaces', baseName, aiInterfaceVersion, 'classes')
#		javaJar = os.path.join(myEnv['builddir'], 'AI/Interfaces', baseName, aiInterfaceVersion, 'interface.jar')
#		javaInterfaceJar = javaJar
#		jlibDir = os.path.join('AI/Interfaces', baseName, 'data', 'jlib')
#		javaInterfaceCP = createJavaClasspath(jlibDir)
#		myEnv['JAVACLASSPATH'] = javaInterfaceCP
#		myEnv['JAVASOURCEPATH'] = javaSrc
#		myClasses = myEnv.Java(target = javaClasses, source = javaSrc)
#		myEnv['JARCHDIR'] = [javaClasses]
#		myJar = myEnv.Jar(target = javaJar, source = myClasses)
#		Alias(baseName, myJar)
#		Alias('AIInterfaces', myJar)
#		Default(myJar)
#		instList += [env.Install(install_data_interface_dir, myJar)]
#		#installDataDir(myEnv, install_data_interface_dir, jlibDir, instList)
#
#	#targetName = baseName + '-' + aiInterfaceVersion
#	targetName = baseName
#	lib = myEnv.SharedLibrary(os.path.join(myEnv['builddir'], 'AI/Interfaces', baseName, aiInterfaceVersion, targetName), mySource)
#	Alias(baseName, lib)       # Allow e.g. `scons Java' to compile just that specific AI interface.
#	Alias('AIInterfaces', lib) # Allow `scons AIInterfaces' to compile all AI interfaces.
#	Default(lib)
#	instList += [myEnv.Install(install_data_interface_dir, lib)]
#	if myEnv['strip']:
#		myEnv.AddPostAction(lib, Action([['strip','$TARGET']]))
#
#	# record data files (eg InterfaceInfo.lua or config files) for installation
#	source_data_dir = os.path.join('AI/Interfaces', baseName, 'data')
#	installDataDir(myEnv, install_data_interface_dir, source_data_dir, instList)
#
#	Alias('install', instList)
#	Alias('install-AIInterfaces', instList)
#	Alias('install-' + baseName, instList)
#
#################################################################################
#### Build Skirmish AI shared objects
#################################################################################
#install_skirmishai_dir = os.path.join(skirmishaienv['installprefix'], skirmishaienv['datadir'], 'AI/Skirmish')
#
## store shared ai objects so newer scons versions don't choke with
## *** Two environments with different actions were specified for the same target
#skirmishaiobjs_main = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source(skirmishaienv), '-skirmishai')
#skirmishaiobjs_mainCregged = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source(skirmishaienv), '-skirmishai_creg', ['USING_CREG'])
#skirmishaiobjs_creg = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source_Creg(skirmishaienv), '-skirmishai_creg', ['USING_CREG'])
#skirmishaiobjs_LegacyCpp = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source_LegacyCpp(skirmishaienv), '-skirmishai')
#skirmishaiobjs_LegacyCppCregged = create_shared_objects(skirmishaienv, filelist.get_shared_skirmishAI_source_LegacyCpp(skirmishaienv), '-skirmishai_creg', ['USING_CREG'])
#
## Build
#skirmishai_exclude_list=['build', 'CSAI', 'TestABICAI', 'AbicWrappersTestAI']
## for some strange reason, NTai has a compile error
## when compiling with MinGW on windows, because of
## a class in the Legacy C++ wrapper, which is used
## by all other legacy C++ AIs aswell
#if sys.platform == 'win32':
#	skirmishai_exclude_list += ['NTai']
#skirmishai_isLegacyCpp_list=['AAI', 'KAIK', 'RAI', 'NullLegacyCppAI', 'KAI', 'NTai']
#skirmishai_needCreg_list=['KAIK', 'KAI']
#skirmishai_isJava_list=['NullJavaAI', 'NullOOJavaAI']
#for baseName in filelist.list_skirmishAIs(skirmishaienv, exclude_list=skirmishai_exclude_list):
#	aiVersion = fetch_ai_version(baseName, 'Skirmish')
#	print "Found Skirmish AI: " + baseName + " " + aiVersion
#	isLegacyCpp = baseName in skirmishai_isLegacyCpp_list
#	useCreg = baseName in skirmishai_needCreg_list
#	isJava = baseName in skirmishai_isJava_list
#	myEnv = skirmishaienv.Clone()
#	install_data_ai_dir = os.path.join(install_skirmishai_dir, baseName, aiVersion)
#	instList = []
#
#	# create the library
#	if isJava:
#		javaSrc = os.path.join('AI/Skirmish', baseName)
#		javaClasses = os.path.join(myEnv['builddir'], 'AI/Skirmish', baseName, aiVersion, 'classes')
#		javaJar = os.path.join(myEnv['builddir'], 'AI/Skirmish', baseName, aiVersion, 'ai.jar')
#		jlibDir = os.path.join('AI/Skirmish', baseName, 'data', 'jlib')
#		aiCP = createJavaClasspath(jlibDir)
#		aiFullCP = aiCP + os.pathsep + javaInterfaceJar + os.pathsep + javaInterfaceCP
#		myEnv['JAVACLASSPATH'] = aiFullCP
#		myEnv['JAVASOURCEPATH'] = javaSrc
#		myClasses = myEnv.Java(target = javaClasses, source = javaSrc)
#		myEnv['JARCHDIR'] = [javaClasses]
#		myJar = myEnv.Jar(target = javaJar, source = myClasses)
#		Alias(baseName, myJar)
#		Alias('SkirmishAI', myJar)
#		Default(myJar)
#		instList += [env.Install(install_data_ai_dir, myJar)]
#		#installDataDir(myEnv, install_data_ai_dir, jlibDir, instList)
#
#	else:
#		if useCreg:
#			myEnv.AppendUnique(CPPDEFINES = ['USING_CREG'])
#		objs = []
#		if useCreg:
#			objs += skirmishaiobjs_mainCregged
#			objs += skirmishaiobjs_creg
#		else:
#			objs += skirmishaiobjs_main
#		if isLegacyCpp:
#			myEnv.AppendUnique(CXXFLAGS = ['-IAI/Wrappers'])
#			if useCreg:
#				objs += skirmishaiobjs_LegacyCppCregged
#			else:
#				objs += skirmishaiobjs_LegacyCpp
#		mySource = objs + filelist.get_skirmishAI_source(myEnv, baseName)
#		#targetName = baseName + '-' + aiVersion
#		targetName = baseName
#		lib = myEnv.SharedLibrary(os.path.join(myEnv['builddir'], 'AI/Skirmish', baseName, aiVersion, targetName), mySource)
#		Alias(baseName, lib)     # Allow e.g. `scons JCAI' to compile just a skirmish AI.
#		Alias('SkirmishAI', lib) # Allow `scons SkirmishAI' to compile all skirmishAIs.
#		Default(lib)
#		instList += [env.Install(install_data_ai_dir, lib)]
#		if myEnv['strip']:
#			myEnv.AddPostAction(lib, Action([['strip','$TARGET']]))
#
#	# record data files (eg AIInfo.lua or config files) for installation
#	source_data_dir = os.path.join('AI/Skirmish', baseName, 'data')
#	installDataDir(myEnv, install_data_ai_dir, source_data_dir, instList)
#
#	# install everything from this AI
#	Alias('install', instList)
#	Alias('install-SkirmishAI', instList)
#	Alias('install-' + baseName, instList)
#
### install AAI config files
##aai_data=filelist.list_files_recursive(env, 'game/AI/AAI')
##for f in aai_data:
##	if not os.path.isdir(f):
##		inst = env.Install(os.path.join(skirmishaienv['installprefix'], skirmishaienv['datadir'], os.path.dirname(f)[5:]), f)
##		Alias('install', inst)
#
#################################################################################
#### Build Group AI shared objects
#################################################################################
##install_groupai_lib_dir = os.path.join(groupaienv['installprefix'], groupaienv['libdir'], 'AI/Helper-libs')
###install_groupai_data_dir = os.path.join(groupaienv['installprefix'], groupaienv['datadir'], 'AI/Helper-libs/data')
##
### store shared ai objects so newer scons versions don't choke with
### *** Two environments with different actions were specified for the same target
##groupaiobjs_main = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source(groupaienv), '-groupai')
##groupaiobjs_mainCregged = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source(groupaienv), '-groupai_creg', ['USING_CREG'])
##groupaiobjs_creg = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source_Creg(groupaienv), '-groupai_creg', ['USING_CREG'])
##groupaiobjs_LegacyCpp = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source_LegacyCpp(groupaienv), '-groupai')
##groupaiobjs_LegacyCppCregged = create_shared_objects(groupaienv, filelist.get_shared_groupAI_source_LegacyCpp(groupaienv), '-groupai_creg', ['USING_CREG'])
##
### Build
##groupai_exclude_list=['build']
##groupai_isLegacyCpp_list=['?', '??']
##groupai_needCreg_list=[]
##for baseName in filelist.list_groupAIs(groupaienv, exclude_list=groupai_exclude_list):
##	libName = construct_groupai_libName(baseName) # eg. MetalMaker-1.0
##	print "Group AI: " + libName
##	#TODO remove the True in the next line, uncomment the rest, and actualize groupai_needCreg_list
##	useCreg = True #baseName in groupai_needCreg_list
##	#TODO remove the True in the next line, uncomment the rest, and actualize groupai_isLegacyCpp_list
##	isLegacyCpp = True #baseName in groupai_isLegacyCpp_list
##	myEnv = groupaienv.Clone()
##	if useCreg:
##		myEnv.AppendUnique(CPPDEFINES = ['USING_CREG'])
##	objs = []
##	if useCreg:
##		objs += groupaiobjs_mainCregged
##		objs += groupaiobjs_creg
##	else:
##		objs += groupaiobjs_main
##	if isLegacyCpp:
##		if useCreg:
##			objs += groupaiobjs_LegacyCppCregged
##		else:
##			objs += groupaiobjs_LegacyCpp
##	mySource = objs + filelist.get_groupAI_source(myEnv, baseName)
##	lib = myEnv.SharedLibrary(os.path.join('game/AI/Helper-libs', baseName), mySource)
##	#lib = myEnv.SharedLibrary(os.path.join('game/AI/Helper-libs', libName), mySource)
##	Alias(baseName, lib)         # Allow e.g. `scons CentralBuildAI' to compile just an AI.
##	Alias('GroupAI', lib) # Allow `scons GroupAI' to compile all groupAIs.
##	Default(lib)
##	inst = env.Install(install_groupai_lib_dir, lib)
##	Alias('install', inst)
##	Alias('install-GroupAI', inst)
##	Alias('install-'+baseName, inst)
##	if myEnv['strip']:
##		myEnv.AddPostAction(lib, Action([['strip','$TARGET']]))
#
#

aienv = env.Clone(builddir=os.path.join(env['builddir'], 'AI'))
remove_precompiled_header(aienv)
env.SConscript(['AI/SConscript'], exports=['env', 'aienv'], variant_dir=env['builddir'])

################################################################################
### Run Tests
################################################################################

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
### Build gamedata zip archives & misc.
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

