# Copyright (C) 2006  Tobi Vollebregt

# see rts/build/scons/*.py for the core of the build system

""" Available targets.
Each target has an equivalent install target. E.g. `CentralBuildAI' has
`install-CentralBuildAI' and the default target has `install'.

[default]
	spring
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


env = Environment(tools = ['default', 'rts'], toolpath = ['.', 'rts/build/scons'])

spring_files = filelist.get_spring_source(env)
# Build Lesson2.cpp separately from the other sources.  This is to prevent recompilation of
# the entire source if one wants to change just the install prefix (and hence the datadir).
Lesson2 = env.Object(os.path.join(env['builddir'], 'rts/System/Lesson2.cpp'),
	CPPDEFINES = env['CPPDEFINES']+['SPRING_DATADIR="\\"'+env['datadir']+'\\""'])
spring = env.Program('game/spring', spring_files + [Lesson2])
Alias('spring', spring)
Default(spring)
inst = Install(os.path.join(env['prefix'], 'bin'), spring)
Alias('install', inst)
Alias('install-spring', inst)

# Make a copy of the build environment for the AIs, but remove libraries and add include path.
aienv = env.Copy(LIBS=[], LIBPATH=[])
aienv.Append(CPPPATH = ['rts/ExternalAI'])

# Use subst() to substitute $prefix in datadir.
install_dir = os.path.join(aienv.subst(aienv['datadir']), 'aidll')

#Build GroupAIs
for f in filelist.list_groupAIs(aienv):
	lib = aienv.SharedLibrary(os.path.join('game/aidll/', f), filelist.get_groupAI_source(aienv, f))
	Alias(f, lib)         # Allow e.g. `scons CentralBuildAI' to compile just an AI.
	Alias('GroupAI', lib) # Allow `scons GroupAI' to compile all groupAIs.
	Default(lib)
	inst = Install(os.path.join(install_dir, 'aidll'), lib)
	Alias('install', inst)
	Alias('install-GroupAI', inst)
	Alias('install-'+f, inst)

#Build GlobalAIs
for f in filelist.list_globalAIs(aienv):
	lib = aienv.SharedLibrary(os.path.join('game/aidll/globalai/', f), filelist.get_globalAI_source(aienv, f))
	Alias(f, lib)          # Allow e.g. `scons JCAI' to compile just a global AI.
	Alias('GlobalAI', lib) # Allow `scons GlobalAI' to compile all globalAIs.
	Default(lib)
	inst = Install(os.path.join(install_dir, 'aidll/globalai'), lib)
	Alias('install', inst)
	Alias('install-GlobalAI', inst)
	Alias('install-'+f, inst)
