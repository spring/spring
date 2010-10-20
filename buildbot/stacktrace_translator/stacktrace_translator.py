#!/usr/bin/env python
# Author: Tobi Vollebregt
# Thanks to bibim for providing the perl source of his translator.


import os, re, sys
import logging
import traceback
from tempfile import NamedTemporaryFile
from subprocess import Popen, PIPE


# Paths to required helper programs.
ADDR2LINE = r'/usr/bin/i586-mingw32msvc-addr2line'
SEVENZIP = r'/usr/bin/7za'

# Everything before the first occurence of this is stripped
# from paths returned by addr2line.
# First one is buildbot, second one is BuildServ.
PATH_STRIP_UNTIL = ['/build/', '}.mingw32.cmake/']

# Root of the directory tree with debugging symbols.
# Must contain paths of the form config/branch/rev/...
WWWROOT = os.path.expanduser('~/www')

# Where to put the log & pid file when running as server.
#LOGFILE = os.path.expanduser('~/log/stacktrace_translator.log')   # unused currently
PIDFILE = os.path.expanduser('~/run/stacktrace_translator.pid')

# Object passed into the XMLRPC server object to listen on.
LISTEN_ADDR = ('', 8000)

# Match common pre- and suffix on infolog lines.
RE_PREFIX = r'^(?:\[\s*\d+\]\s+)?'
RE_SUFFIX = '\r?$'

# Match stackframe lines, captures the module name and the address.
# Example: '[      0] (0) C:\Program Files\Spring\spring.exe [0x0080F268]'
#          -> ('C:\\Program Files\\Spring\\spring.exe', '0x0080F268')
RE_STACKFRAME = re.compile(RE_PREFIX + r'\(\d+\)\s+(.*(?:\.exe|\.dll))(?:\([^)]*\))?\s+\[(0x[\dA-Fa-f]+)\]' + RE_SUFFIX, re.MULTILINE)

# Match complete version string, capture `Additional' version string.
RE_VERSION = r'Spring [^\(]+ \(([^\)]+)\)[\w\(\) ]*'
RE_OLD_VERSION = r'Spring [^\(]+ \(([^\)]+)\{\@\}-cmake-mingw32\)[\w\(\) ]*'

# Match complete line containing version string.
RE_VERSION_LINES = [
	re.compile(x % (RE_PREFIX, RE_VERSION, RE_SUFFIX), re.MULTILINE) for x in
	[r'%s%s has crashed\.%s', r'%sHang detection triggered for %s\.%s']
]
RE_OLD_VERSION_LINES = [
	re.compile(x % (RE_PREFIX, RE_OLD_VERSION, RE_SUFFIX), re.MULTILINE) for x in
	[r'%s%s has crashed\.%s', r'%sHang detection triggered for %s\.%s']
]

# Capture config, branch, rev from `Additional' version string.
RE_CONFIG = r'(?:\[(?P<config>\w+)\])?'
RE_BRANCH = r'(?:\{(?P<branch>\w+)\})?'
RE_REV = r'(?P<rev>[0-9.]+(?:-[0-9]+-g[0-9A-Fa-f]+)?)'
RE_VERSION_DETAILS = re.compile(RE_CONFIG + RE_BRANCH + RE_REV + r'\s')
# Same regex is safe, but can't be used to detect config or branch in old version strings.
RE_OLD_VERSION_DETAILS = RE_VERSION_DETAILS

# Match filename of file with debugging symbols, capture module name.
RE_DEBUG_FILENAME = re.compile(RE_CONFIG + RE_BRANCH + RE_REV + r'_(?P<module>[-\w]+)_dbg.7z')

# Set up application log.
log = logging.getLogger('stacktrace_translator')
log.setLevel(logging.DEBUG)


class FatalError(Exception):
	'''The only exception that doesn't trigger a dump of a trace server-side.'''
	def __init__(self, message):
		Exception.__init__(self, message)


def fatal(message):
	'''A fatal error happened, quit the translation process.'''
	log.error(message)   # for server
	raise FatalError(message)   # for client


def common_suffix_length(a, b):
	'''\
	Compute the length of the common suffix of a and b, considering / and \ equal.

		>>> common_suffix_length('/foo/bar/baz', '\\\\other\\\\bar\\\\baz')
		8
		>>> common_suffix_length('bar', 'foo/bar')
		3
		>>> common_suffix_length('bar', 'bar')
		3
	'''
	a = a.replace('\\', '/')
	b = b.replace('\\', '/')
	min_length = min(len(a), len(b))
	for i in range(-1, -min_length - 1, -1):
		if a[i] != b[i]: return -i - 1
	return min_length


def best_matching_module(needle, haystack):
	'''\
	Choose the best matching module, based on longest common suffix.

	This way it ignores the install location of Spring:

		>>> modules = ['spring.exe', 'AI/Skirmish/NullAI/SkirmishAI.dll']
		>>> best_matching_module('c:/Program Files/Spring/spring.exe', modules)
		'spring.exe'

	Unfortunately if the correct module isn't available the closest match can be wrong:

		>>> best_matching_module('c:/Program Files/Spring/AI/Skirmish/UnknownAI/SkirmishAI.dll', modules)
		'AI/Skirmish/NullAI/SkirmishAI.dll'
	'''
	longest_csl = 0
	best_match = None
	for module in haystack:
		csl = common_suffix_length(needle, module)
		if csl > longest_csl:
			longest_csl = csl
			best_match = module
	return best_match


def detect_version_helper(infolog, re_version_lines, re_version_details):
	log.info('Detecting version details...')

	version = None
	for re_version_line in re_version_lines:
		match = re_version_line.search(infolog)
		if match:
			version = match.group(1)
			break
	else:
		fatal('Unable to find detailed version in infolog')

	# a space is added so the regex can check for (end of string | space) easily.
	match = re_version_details.match(version + ' ')
	if not match:
		fatal('Unable to parse detailed version string "%s"' % version)

	config, branch, rev = match.group('config', 'branch', 'rev')
	if not config: config = 'default'
	if not branch: branch = 'master'

	log.info('\t[OK] config = %s, branch = %s, rev = %s', config, branch, rev)
	return config, branch, rev


def detect_old_version_details(infolog):
	'''\
	Detect config, branch, rev from BuildServ version string in infolog.

	Detects at least the latest official releases from before the switch to buildbot:

		>>> detect_old_version_details('Spring 0.81.2.1 (0.81.2.1-0-g884a107{@}-cmake-mingw32) has crashed.')
		('default', 'master', '0.81.2.1-0-g884a107')
		>>> detect_old_version_details('Spring 0.81.2.0 (0.81.2-0-g76e4cf5{@}-cmake-mingw32) has crashed.')
		('default', 'master', '0.81.2-0-g76e4cf5')

	It should not detect new-style version strings:

		>>> detect_old_version_details('Spring 0.81+.0.0 (0.81.2.1-1059-g7937d00) has crashed.')
		Traceback (most recent call last):
			...
		FatalError: Unable to find detailed version in infolog
	'''
	return detect_version_helper(infolog, RE_OLD_VERSION_LINES, RE_OLD_VERSION_DETAILS)


def detect_version_details(infolog):
	'''\
	Detect config, branch, rev from version string(s) in infolog.

	These should be fine:

		>>> detect_version_details('Spring 0.81+.0.0 (0.81.2.1-1059-g7937d00) has crashed.')
		('default', 'master', '0.81.2.1-1059-g7937d00')

		>>> detect_version_details('Hang detection triggered for Spring 0.81+.0.0 ([debug2]{pyAiInt}0.81.2.1-1059-g7937d00).')
		('debug2', 'pyAiInt', '0.81.2.1-1059-g7937d00')

	This is an old-style (BuildServ) version string, it should be rejected:

		>>> detect_version_details('Spring 0.81.2.1 (0.81.2.1-0-g884a107{@}-cmake-mingw32) has crashed.')
		Traceback (most recent call last):
			...
		FatalError: Unable to parse detailed version string "0.81.2.1-0-g884a107{@}-cmake-mingw32"
	'''
	return detect_version_helper(infolog, RE_VERSION_LINES, RE_VERSION_DETAILS)


def collect_stackframes(infolog):
	'''\
	Collect stackframes from infolog, grouped by module.
	(because addr2line has a huge per-module overhead)

		>>> collect_stackframes('(0) C:/spring.exe [0x0080F268]')
		({'C:/spring.exe': [(0, '0x0080F268')]}, 1)
	'''
	log.info('Collecting stackframes per module...')

	frames = {}
	frame_count = 0
	for module, address in RE_STACKFRAME.findall(infolog):
		frames.setdefault(module, []).append((frame_count, address))
		frame_count += 1

	log.debug('frames = %s, frame_count = %d', frames, frame_count)
	log.info('\t[OK]')
	return frames, frame_count


def collect_modules(config, branch, rev, buildserv):
	'''\
	Collect modules for which debug data is available.
	Return dict which maps (simplified) module name to debug symbol filename.
	'''
	log.info('Checking debug data availability...')

	if buildserv:
		dir = os.path.join(WWWROOT, 'buildserv', config, branch, rev)
	else:
		dir = os.path.join(WWWROOT, config, branch, rev)

	if not os.path.isdir(dir):
		fatal('No debugging symbols available')

	modules = {}
	for filename in os.listdir(dir):
		match = RE_DEBUG_FILENAME.match(filename)
		if match:
			module = match.groupdict()['module']
			path = os.path.join(dir, filename)
			if module.startswith('spring'):
				# executables
				modules[module + '.exe'] = path
			elif module == 'unitsync':
				# 'normal' DLLs
				modules[module + '.dll'] = path
			else:
				# AI DLLs
				modules['%s/SkirmishAI.dll' % module] = path

	log.debug('modules = %s', modules)
	log.info('\t[OK]')
	return modules


def translate_module_addresses(module, debugfile, addresses):
	'''\
	Translate addresses in a module to (module, address, filename, lineno) tuples
	by invoking addr2line exactly once on the debugging symbols for that module.
	'''
	with NamedTemporaryFile() as tempfile:
		log.info('\tExtracting debug symbols for module %s from archive %s...' % (module, os.path.basename(debugfile)))
		# e = extract without path, -so = write output to stdout, -y = yes to all questions
		sevenzip = Popen([SEVENZIP, 'e', '-so', '-y', debugfile], stdout = tempfile, stderr = PIPE)
		stdout, stderr = sevenzip.communicate()
		if stderr:
			log.debug('%s stderr: %s' % (SEVENZIP, stderr))
		if sevenzip.returncode != 0:
			fatal('%s exited with status %s' % (SEVENZIP, sevenzip.returncode))
		log.info('\t\t[OK]')

		log.info('\tTranslating addresses for module %s...' % module)
		addr2line = Popen([ADDR2LINE, '-j', '.text', '-e', tempfile.name], stdin = PIPE, stdout = PIPE, stderr = PIPE)
		if addr2line.poll() == None:
			stdout, stderr = addr2line.communicate('\n'.join(addresses))
		else:
			stdout, stderr = addr2line.communicate()
		if stderr:
			log.debug('%s stderr: %s' % (ADDR2LINE, stderr))
		if addr2line.returncode != 0:
			fatal('%s exited with status %s' % (ADDR2LINE, addr2line.returncode))
		log.info('\t\t[OK]')

	def fixup(addr, file, line):
		for psu in PATH_STRIP_UNTIL:
			if psu in file:
				file = file[file.index(psu)+len(psu):]
				break
		return module, addr, file, int(line)

	return [fixup(addr, *line.split(':')) for addr, line in zip(addresses, stdout.splitlines())]


def translate_(module_frames, frame_count, modules):
	'''\
	Translate the stacktrace given in (module_frames, frame_count) by repeatedly
	invoking addr2line on the debugging data for the modules.
	'''
	log.info('Translating stacktrace...')

	module_names = modules.keys()
	translated_stacktrace = [None] * frame_count
	for module, frames in module_frames.iteritems():
		module_name = best_matching_module(module, module_names)
		indices, addrs = zip(*frames)   # unzip
		if module_name:
			translated_frames = translate_module_addresses(module, modules[module_name], addrs)
			for index, translated_frame in zip(indices, translated_frames):
				translated_stacktrace[index] = translated_frame
		else:
			for i in range(len(indices)):
				translated_stacktrace[indices[i]] = (module, addrs[i], '??', 0)   # unknown

	log.debug('translated_stacktrace = %s', translated_stacktrace)
	log.info('\t[OK]')
	return translated_stacktrace


def translate_stacktrace(infolog):
	r'''\
	Translate a complete stacktrace to (module, address, filename, lineno) tuples.

	The input string may be a complete infolog (i.e. infolog.txt). At the very
	least it must contain the 'Spring XXX has crashed.' or 'Hang detection
	triggered for Spring XXX.' line and at least one stack frame.

	The output is a list of (module, address, filename, lineno) tuples,
	or (module, address, '??', 0) for each frame that could not be translated.

	Example of a remote call to the service in Python:
	(Note that tuples have become lists)

		>>> from xmlrpclib import ServerProxy   #doctest:+SKIP
		... proxy = ServerProxy('http://springrts.com:8000/')
		... proxy.translate_stacktrace(file('infolog.txt').read())
		[['C:\\Program Files\\Spring\\spring.exe', '0x0080F6F8', 'rts/Rendering/Env/GrassDrawer.cpp', 229],
		 ['C:\\Program Files\\Spring\\spring.exe', '0x008125DF', 'rts/Rendering/Env/GrassDrawer.cpp', 136],
		 ['C:\\Program Files\\Spring\\spring.exe', '0x00837E8C', 'rts/Rendering/Env/AdvTreeDrawer.cpp', 54],
		 ['C:\\Program Files\\Spring\\spring.exe', '0x0084189E', 'rts/Rendering/Env/BaseTreeDrawer.cpp', 57],
		 ['C:\\Program Files\\Spring\\spring.exe', '0x00402AA8', 'rts/Game/Game.cpp', 527],
		 ...
		 ['C:\\WINDOWS\\system32\\kernel32.dll(RegisterWaitForInputIdle+0x49)', '0x7C7E7077', '??', 0]]

	Example of a local call:

		>>> translate_stacktrace(file('infolog.txt').read())
		[('C:\\Program Files\\Spring\\spring.exe', '0x0080F6F8', 'rts/Rendering/Env/GrassDrawer.cpp', 229),
		 ('C:\\Program Files\\Spring\\spring.exe', '0x008125DF', 'rts/Rendering/Env/GrassDrawer.cpp', 136),
		 ('C:\\Program Files\\Spring\\spring.exe', '0x00837E8C', 'rts/Rendering/Env/AdvTreeDrawer.cpp', 54),
		 ('C:\\Program Files\\Spring\\spring.exe', '0x0084189E', 'rts/Rendering/Env/BaseTreeDrawer.cpp', 57),
		 ('C:\\Program Files\\Spring\\spring.exe', '0x00402AA8', 'rts/Game/Game.cpp', 527),
		 ...
		 ('C:\\WINDOWS\\system32\\kernel32.dll(RegisterWaitForInputIdle+0x49)', '0x7C7E7077', '??', 0)]
	'''

	log.info('----- Start of translation process -----')

	# TODO: add module checksum dump to spring, add code here to parse those.

	# With checksums, the idea is:
	# 1) put module_names and module_checksums in a dict which maps name to checksum
	# 2) use best_matching_module to find best matching item from module_names
	# 3) get its checksum
	# 4) download debugging symbols from ##/#############################, which
	#    shall be a symlink to the correct 7z file containing debugging symbols.
	# 5) perform translation..

	# Without checksums, the idea is:
	# 1) look in config/branch/rev folder
	# 2) build a list of modules based on *_dbg.7z files available in this folder
	#    (there's no guarantee of course that AI's, unitsync, etc. are of the
	#    same version as Spring.., but we'll have to live with that.)
	# 3) download debugging symbols for a module directly from config/branch/rev/...
	# 4) perform translation

	try:
		# Detected old style version string?
		buildserv = False

		try:
			config, branch, rev = detect_version_details(infolog)
		except FatalError:
			# Try old version string, we might have debug symbols for some of them
			config, branch, rev = detect_old_version_details(infolog)
			buildserv = True

		module_frames, frame_count = collect_stackframes(infolog)

		# Hack to be able to translate symbols from Spring 0.82.6 and 0.82.6.1
		# These symbols have had 0x400000 subtracted from them (within spring)
		# module_frames = {'spring.exe': [(0, '0x1234'), (1, '0x2345')]
		if rev == '0.82.6' or rev == '0.82.6.0' or rev == '0.82.6.1':
			log.debug('Applying workaround for Spring 0.82.6 and 0.82.6.1')
			for module, frames in module_frames.iteritems():
				if re.search(r'spring(-.*)?.exe', module, re.IGNORECASE):
					module_frames[module] = [(i, hex(int(addr, 16) + 0x400000)) for i, addr in frames]

		modules = collect_modules(config, branch, rev, buildserv)

		translated_stacktrace = translate_(module_frames, frame_count, modules)

	except FatalError:
		# FatalError is intended to reach the client => re-raise
		raise

	except Exception:
		# Log the real exception
		log.critical(traceback.format_exc())
		# Throw a new exception without leaking too much information
		raise FatalError('unhandled exception')

	log.info('----- End of translation process -----')
	return translated_stacktrace


def run_xmlrpc_server():
	'''Run an XMLRPC server that publishes the translate_stacktrace function.'''
	from DocXMLRPCServer import DocXMLRPCServer as XMLRPCServer

	logging.basicConfig(format='%(asctime)s - %(levelname)s - %(message)s')

	with open(PIDFILE, 'w') as pidfile:
		pidfile.write('%s' % os.getpid())

	try:
		# Create server
		server = XMLRPCServer(LISTEN_ADDR)
		server.register_function(translate_stacktrace)

		# Run the server's main loop
		try:
			server.serve_forever()
		except KeyboardInterrupt:
			pass

	finally:
		os.remove(PIDFILE)


if __name__ == '__main__':
	if len(sys.argv) > 1 and sys.argv[1] == '--test':
		import doctest
		logging.basicConfig(format='%(message)s')
		log.setLevel(logging.INFO)
		doctest.testmod(optionflags = doctest.NORMALIZE_WHITESPACE + doctest.ELLIPSIS)
	else:
		run_xmlrpc_server()
