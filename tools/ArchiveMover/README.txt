Build instructions:

Add the files in “%SVN_Root%/rts/lib/7zip” and “%SVN_Root%/rts/lib/minizip” to the project.
Also add “_SZ_ONE_DIRECTORY” to the projects preprocessor defines.

Download and install Boost 1.34, if you don't want to install everything add the files in “%Boost_Root%/libs/filesystem/src” to the project and define “BOOST_FILESYSTEM_NO_LIB”.

You can use the precompiled zlib from “spring-vclibs” or you can download zlib and use the source directly.
If you use the precompiled version add “ZLIB_WINAPI” to the projects preprocessor defines.

The MSVC 2005 project file is currently set up to expect the entire boost 1.34.1 source tree in the current directory (and to compile using BOOST_FILESYSTEM_NO_LIB). It expects the vclibs package at the regular place for compiling Spring for the zlibwapi dependency.
