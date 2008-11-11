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


def list_directories(env, path, exclude_list = (), exclude_regexp = '^\.'):
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
					path_stack += [g]
	return dirs


def list_files_recursive(env, path, exclude_list = (), exclude_regexp = '^\.'):
	path_stack = [path]
	exclude = re.compile(exclude_regexp)
	ffiles = []
	while len(path_stack) > 0:
		path = path_stack.pop()
		files = os.listdir(path)
		for f in files:
			g = os.path.join(path, f)
			if os.path.exists(g) and not f in exclude_list and not exclude.search(f):
				ffiles += [g]
				if os.path.isdir(g):
					path_stack += [g]
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


def get_AI_source(env, path, which):
	return get_source(env, os.path.join(path, which))


def get_globalAI_source(env, which):
	return get_AI_source(env, 'AI/Global', which)


def get_groupAI_source(env, which):
	return get_AI_source(env, 'AI/Group', which)


def get_shared_AI_source(env):
	result = get_source(env, 'rts/System/creg')
	if env.has_key('builddir') and env['builddir']:
		result += [os.path.join(env['builddir'], 'rts/System/float3.cpp')]
	return result

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


def list_globalAIs(env, exclude_list = (), exclude_regexp = '^\.'):
	return list_AIs(env, 'AI/Global', exclude_list, exclude_regexp)


def list_groupAIs(env, exclude_list = (), exclude_regexp = '^\.'):
	return list_AIs(env, 'AI/Group', exclude_list, exclude_regexp)
