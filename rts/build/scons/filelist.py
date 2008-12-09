# Copyright (C) 2006  Tobi Vollebregt

import os, re

###############################################
### functions to make lists of source files ###
###############################################


def fix_path(path):
	pieces = path.split('/')
	newpath = None
	for p in pieces:
		if newpath:
			newpath = os.path.join(newpath, p)
		else:
			newpath = p
	return newpath


def list_directories(env, path, exclude_list = (), exclude_regexp = '^\.', recursively = True):
	path_stack = [path]
	#walk through dir tree
	exclude = re.compile(exclude_regexp)
	dirs = []
	while len(path_stack) > 0:
		path = path_stack.pop()
		files = os.listdir(path)
		for f in files:
			g = os.path.join(path, f)
			if os.path.exists(g) and not f in exclude_list and not exclude.search(f):
				if os.path.isdir(g):
					dirs += [g]
					if recursively:
						path_stack += [g]
	return dirs


def list_files_recursive(env, path, exclude_list = (), exclude_regexp = '^\.', exclude_dirs = False, path_relative = False):
	rel_path_stack = ['']
	exclude = re.compile(exclude_regexp)
	ffiles = []
	while len(rel_path_stack) > 0:
		rpath = rel_path_stack.pop()
		apath = os.path.join(path, rpath)
		files = os.listdir(apath)
		for f in files:
			rf = os.path.join(rpath, f)
			af = os.path.join(apath, f)
			if path_relative:
				wf = rf
			else:
				wf = af
			if os.path.exists(af) and not f in exclude_list and not exclude.search(f):
				if not os.path.isdir(af) or not exclude_dirs:
					ffiles += [wf]
				if os.path.isdir(af):
					rel_path_stack += [rf]
	return ffiles


def get_source(env, path, exclude_list = (), exclude_regexp = '^\.'):
	basepath = path
	path_stack = [path]
	#what does a source file look like?
	source = re.compile('\.cpp$|\.c$')
	#walk through dir tree
	exclude = re.compile(exclude_regexp)
	source_files = []
	while len(path_stack) > 0:
		path = path_stack.pop()
		files = os.listdir(path)
		for f in files:
			g = os.path.join(path, f)
			if os.path.exists(g) and not g in exclude_list and not exclude.search(f):
				if source.search(f)  and os.path.isfile(g):
					source_files += [g]
				# Exclude 'Test' too so one can dump unit testing code in 'Test' subdirectories
				# (Couldn't really use regexp for it since it'd exclude files like TestScript.cpp too)
				elif os.path.isdir(g) and f.lower() != 'test':
					path_stack += [g]
	if env.has_key('builddir') and env['builddir']:
		source_files_2 = []
		for f in source_files:
			source_files_2.append([os.path.join(env['builddir'], f)])
		return source_files_2
	else:
		return source_files


def get_spring_source(env):
	exclude1 = [
		'rts/AI',
		'rts/build',
		'rts/lib/crashrpt', # unused
		'rts/lib/libhpi',
		'rts/lib/streflop', # streflop is compiled with it's own Makefiles
		'rts/System/Platform/BackgroundReader.cpp',
		'rts/System/Platform/Mac', # Mac build uses XCode
		'rts/System/FileSystem/DataDirLocater.cpp', # see SConstruct
		#'rts/ExternalAI/Interface/LegacyCppWrapper', # only needed by AI libraries using the legacy C++ interface to connect with spring
	]
	# we may be called before we were configured (e.g. when cleaning)
	if env.has_key('platform'):
		if env['platform'] == 'windows':
			exclude1 += [
				'rts/System/Platform/Linux',
				'rts/Rendering/GL/GLXPBuffer.cpp']
				#'rts/System/Platform/Win/DxSound.cpp']
		else:
			exclude1 += [
				'rts/Rendering/GL/WinPBuffer.cpp', # why not in `System/Win/'?
				'rts/lib/minizip/iowin32.c',
				'rts/System/Platform/Win',
				'rts/System/wavread.cpp']
		if env['disable_avi']: exclude1 += ['rts/System/Platform/Win/AVIGenerator.cpp']

	# for Windows we must add the backslash equivalents
	exclude = []
	for f in exclude1:
		exclude += [f]
		exclude += [f.replace('/','\\')]

	source = get_source(env, 'rts', exclude_list = exclude)
	return source

################################################################################
### AI
################################################################################

# lists source files for each individual AI Interface, Skirmish and Group AI
def get_AI_source(env, path, which):
	result = get_source(env, os.path.join(path, which))
	return result

# lists source files for each individual AI Interface
def get_AIInterface_source(env, which):
	result = get_AI_source(env, 'AI/Interfaces', which)
	return result

# lists source files for each individual Skirmish AI
def get_skirmishAI_source(env, which):
	result = get_AI_source(env, 'AI/Skirmish', which)
	return result

# lists source files for each individual Group AI
def get_groupAI_source(env, which):
	result = get_AI_source(env, 'AI/Group', which)
	return result


# lists source files common for all AI Interfaces, Skirmish and Group AIs
def get_shared_AI_source(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += [os.path.join(env['builddir'], 'rts/ExternalAI/Interface/SSkirmishAISpecifier.cpp')]
		result += [os.path.join(env['builddir'], 'rts/Game/GameVersion.cpp')]
#		result += [os.path.join(env['builddir'], 'rts/System/Platform/errorhandler.cpp')]
	return result

# lists source files common for all AI Interfaces
def get_shared_AIInterface_source(env):
	result = get_shared_AI_source(env)
	return result
# list SharedLib source files (used by some AI Interface libraries)
def get_shared_AIInterface_source_SharedLib(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += [os.path.join(env['builddir'], 'rts/System/Platform/SharedLib.cpp')]
		if env['platform'] == 'windows':
			result += [os.path.join(env['builddir'], 'rts/System/Platform/Win/DllLib.cpp')]
		else:
			result += [os.path.join(env['builddir'], 'rts/System/Platform/Linux/SoLib.cpp')]
	return result

# lists source files common for all Skirmish AIs
def get_shared_skirmishAI_source(env):
	result = get_shared_AI_source(env)
	return result
# list LegacyCpp source files (used by some Skirmish AI libraries)
def get_shared_skirmishAI_source_LegacyCpp(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += [os.path.join(env['builddir'], 'rts/System/float3.cpp')]
		result += [os.path.join(env['builddir'], 'rts/Sim/Misc/DamageArray.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AISCommands.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AIAICallback.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AIAICheats.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AIGlobalAICallback.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AIGlobalAI.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AI.cpp')]
	return result
# list Creg source files (used by some Skirmish AI libraries)
def get_shared_skirmishAI_source_Creg(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += get_source(env, 'rts/System/creg')
	return result

# lists source files common for all Group AIs
def get_shared_groupAI_source(env):
	result = get_shared_AI_source(env)
	return result
# list LegacyCpp source files (used by some Group AI libraries)
def get_shared_groupAI_source_LegacyCpp(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += [os.path.join(env['builddir'], 'rts/System/float3.cpp')]
		result += [os.path.join(env['builddir'], 'rts/Sim/Misc/DamageArray.cpp')]
		result += [os.path.join(env['builddir'], 'AI/Wrappers/LegacyCpp/AISCommands.cpp')]
	return result
# list Creg source files (used by some Group AI libraries)
def get_shared_groupAI_source_Creg(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += get_source(env, 'rts/System/creg')
	return result


# lists source directories for AI Interfaces, Skirmish or Group AIs
def list_AIs(env, path, exclude_list = (), exclude_regexp = '^\.'):
	exclude = re.compile(exclude_regexp)
	files = os.listdir(path)
	result = []
	for f in files:
		g = os.path.join(path, f)
		if os.path.exists(g) and not f in exclude_list and not exclude.search(f):
			if os.path.isdir(g):
				result += [f]
	return result

# lists source directories for AI Interfaces
def list_AIInterfaces(env, exclude_list = (), exclude_regexp = '^\.'):
	return list_AIs(env, 'AI/Interfaces', exclude_list, exclude_regexp)

# lists source directories for Skirmish AIs
def list_skirmishAIs(env, exclude_list = (), exclude_regexp = '^\.'):
	return list_AIs(env, 'AI/Skirmish', exclude_list, exclude_regexp)

# lists source directories for Group AIs
def list_groupAIs(env, exclude_list = (), exclude_regexp = '^\.'):
	return list_AIs(env, 'AI/Group', exclude_list, exclude_regexp)
