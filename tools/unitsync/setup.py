# Author: Tobi Vollebregt
# Python distutils build script for unitsync.

from distutils.core import setup, Extension
from sys import path, platform

# If this crashes, run 'scons configure' and try again.
path += ['build']
import intopts

# Compile a list of source files.
unitsync_files = [
	'tools/unitsync/pybind.cpp',
	'tools/unitsync/Syncer.cpp',
	'tools/unitsync/SyncServer.cpp',
	'tools/unitsync/unitsync.cpp',
	'rts/System/TdfParser.cpp',
	'rts/System/FileSystem/Archive7Zip.cpp',
	'rts/System/FileSystem/ArchiveBuffered.cpp',
	'rts/System/FileSystem/ArchiveFactory.cpp',
	'rts/System/FileSystem/ArchiveHPI.cpp',
	'rts/System/FileSystem/ArchiveScanner.cpp',
	'rts/System/FileSystem/ArchiveZip.cpp',
	'rts/System/FileSystem/FileHandler.cpp',
	'rts/System/FileSystem/VFSHandler.cpp',
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
if platform == 'win32':
	unitsync_files += ['rts/lib/minizip/iowin32.c']

"""
# Process CPPDEFINES. (Extension() expects them as 1- or 2-element tuples.)
# Disable because we only seem to need _SZ_ONE_DIRECTORY.
cppdefines = []
for define in intopts.CPPDEFINES:
	eq = define.find('=')
	if eq != -1:
		cppdefines += [(define[:eq], define[eq+1:])]
	else:
		cppdefines += [(define, 1)]
print cppdefines
"""

# Setup the unitsync library.
unitsync = Extension (
	name          = 'unitsync',
	define_macros = [('_SZ_ONE_DIRECTORY', 1)],
#	define_macros = cppdefines,
	include_dirs  = intopts.CPPPATH,
	libraries     = intopts.LIBS,
	library_dirs  = intopts.LIBPATH,
	sources       = unitsync_files
)
setup (
	name = 'unitsync',
	version = '0.1',
	description = 'TODO',
	author = 'SY',
	author_email = 'taspring-linux@lolut.utbm.info',
	url = 'http://spring.clan-sy.com/',
	long_description = '''TODO''',
	ext_modules = [unitsync]
)
