# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

import os, re

###############################################
### functions to make lists of source files ###
###############################################
import sys
sourceRootDir = ''
def setSourceRootDir(absPath):
	global sourceRootDir
	print("source root-dir: " + absPath)
	sourceRootDir = absPath
def getAbsDir(env, relPath):
	# This worked up to SCons 0.98
	#return SCons.Script.Dir(interfaceGeneratedJavaSrcDir).abspath
	path = relPath
	if (len(relPath) > 0 and relPath[0:1] == '#'):
		# replace '#' with source root
		#path = os.path.join(sourceRootDir, relPath.replace('#', ''))
		path = os.path.join(sourceRootDir, relPath[1:])
	return os.path.abspath(path)

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


def list_files_recursive(env, path, exclude_list = (), exclude_regexp = '^\.', exclude_dirs = False, path_relative = False, include_regexp = ''):
	rel_path_stack = ['']
	exclude = re.compile(exclude_regexp)
	include = re.compile(include_regexp)
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
				if not (os.path.isdir(af) and exclude_dirs) and include.search(f):
					ffiles += [wf]
				if os.path.isdir(af):
					rel_path_stack += [rf]
	return ffiles


def get_source(env, path, exclude_list = (), exclude_regexp = '^\.', ignore_builddir = False):
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
	if not ignore_builddir and env.has_key('builddir') and env['builddir']:
		source_files_2 = []
		for f in source_files:
			source_files_2.append([os.path.join(env['builddir'], f)])
		return source_files_2
	else:
		return source_files


def get_spring_source(env):
	exclude1 = [
		'rts/build',
		'rts/lib/crashrpt', # unused
		'rts/lib/libhpi',
		'rts/lib/streflop', # compiled separately, as a static lib
		'rts/lib/oscpack',  # compiled separately, as a static lib
		'rts/System/Platform/BackgroundReader.cpp',
		'rts/System/Platform/Mac', # Mac build uses XCode
		'rts/System/FileSystem/DataDirLocater.cpp', # see SConstruct
	]
	# we may be called before we were configured (e.g. when cleaning)
	if env.has_key('platform'):
		if env['platform'] == 'windows':
			exclude1 += [
				'rts/System/Platform/Linux',
				'rts/Rendering/GL/GLXPBuffer.cpp']
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

# lists source files common for all AI Interfaces and Skirmish AIs
def get_shared_AI_source(env):
	result = []
	if env.has_key('builddir') and env['builddir']:
		result += [os.path.join(env['builddir'], 'rts/Game/GameVersion.cpp')]
	return result

# lists source files common for all AI Interfaces
def get_shared_AIInterface_sources(env):
	result = get_shared_AI_source(env)
	return result

# lists source files common for all Skirmish AIs
def get_shared_AILib_sources(env):
	result = get_shared_AI_source(env)
	return result

# list the LegacyCPP source files (used by some Skirmish AI libraries)
def get_shared_AILib_legacyCPP_sources(env):
	result = []

	if (env.has_key('builddir') and env['builddir']):
		result += [os.path.join(env['builddir'], 'rts/System/float3.cpp')]
		result += [os.path.join(env['builddir'], 'rts/Sim/Misc/DamageArray.cpp')]
		result += get_shared_wrapper_source(env, 'LegacyCpp', prefix = 'AI')

	return result

# list the C and C++ source files of a Wrapper
def get_shared_wrapper_source(env, wrapperDir, prefix = ''):
	result = []

	if (env.has_key('builddir') and env['builddir']):
		fullWrapperDir = os.path.join('Wrappers', wrapperDir)
		files = list_files_recursive(env, fullWrapperDir, exclude_dirs = True, path_relative = True, include_regexp="\.(c|cpp)$")
		for f in files:
			result += [os.path.join(env['builddir'], prefix, fullWrapperDir, f)]
			#result += [os.path.join(env['builddir'], 'Wrappers/LegacyCpp/AI.cpp')]

	return result

# list the creg source files (used by some native SkirmishAI libraries)
def get_shared_AILib_creg_sources(env):
	result = []
	cwdPrev = os.getcwd()

	if (env.has_key('builddir') and env['builddir']):
		os.chdir('..')
		result += get_source(env, 'rts/System/creg', ignore_builddir=False)
		os.chdir(cwdPrev)

	return result

# list the sources of Spring's custom Lua lib (used by some native SkirmishAI libraries)
def get_shared_AILib_lua_sources(env):
	result = []
	cwdPrev = os.getcwd()

	if (env.has_key('builddir') and env['builddir']):
		os.chdir('..')
		result += get_source(env, "rts/lib/lua/src", ignore_builddir=False)
		os.chdir(cwdPrev)

	return result



# lists source directories for AI Interfaces or Skirmish AIs
def list_AIs(env, path, exclude_list = (), exclude_regexp = '^\.', include_interfaces = None):
	exclude = re.compile(exclude_regexp)
	files = os.listdir(path)
	result = []
	for f in files:
		g = os.path.join(path, f)
		if (path == 'Skirmish'):
			infoFile = os.path.join(g, 'data', 'AIInfo.lua')
		else:
			infoFile = os.path.join(g, 'data', 'InterfaceInfo.lua')
		if os.path.exists(g) and not f in exclude_list and not exclude.search(f):
			if os.path.isdir(g) and os.path.exists(infoFile):
				if include_interfaces == None:
					result += [f]
				else:
					# only list AIs whichs info lua file sais that it uses one of the include interfaces
					for i in include_interfaces:
						searchStr = "'" + i + "', -- AI Interface name"
						if (searchStr in open(infoFile).read()):
							result += [f]
							break;
	return result

# lists source directories for AI Interfaces
def list_AIInterfaces(env, exclude_list = (), exclude_regexp = '^\.'):
	return list_AIs(env, 'Interfaces', exclude_list, exclude_regexp)

# lists source directories for Skirmish AIs
def list_skirmishAIs(env, exclude_list = (), exclude_regexp = '^\.', include_interfaces = None):
	return list_AIs(env, 'Skirmish', exclude_list, exclude_regexp, include_interfaces)

