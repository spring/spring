Build instructions:

Download and install Boost 1.34, if you don't want to install everything add the files in “%Boost_Root%/libs/filesystem/src” to the project and define “BOOST_FILESYSTEM_NO_LIB”.

The MSVC 2005 project file is currently set up to expect the entire boost 1.34.1 source tree in the current directory (and to compile using BOOST_FILESYSTEM_NO_LIB).