This directory contains (in addition to this file):

- flt-32/ dbl-64/ ldbl-96/ headers/: These subdirectories are imported from the GNU libm automatically. Do not modify them manually. You may safely erase them and run the import.pl script again. The "clean-import" make target does just that.

- import.pl: A perl script that takes as argument the location of a valid glibc-2.4 source directory. It will create the aforementioned subdirectories and convert the original libm files. The conversion mainly concerns removing external dependencies, and making the c files and constructs compilable with the streflop wrapper types. See the import.pl script itself for the full list of modifications.

- Makefile: Instructions for compiling the subdirectories with the make command. You you re-use this as the basis for other build systems (like CMake, scons, etc.).

- streflop_libm_bridge.h: Contains the definitions and other macros that are necessary to compile libm, in a streflop framework. These declarations were either done by external files, or by other parts of the glibc that were not imported.

- e_expf.c: The float exp in e_expf.c uses doubles internally since revision 1.2 in the libm-ieee754 CVS attic! This is the slower, but purely float version, that is rolled back from the CVS attic.

- w_expf.c: A wrapper to expf that replaces the libm wrapper by a wraper to the float only version.

- (after compilation): flt-target dbl-target ldbl-target temporary files for the make process

The original GNU libm is released under the GNU LGPL license, and so are these modifications. See the LGPL.txt in the parent streflop main directory. See also the comments at the beginning of each file for particular information, especially the Sun Microsystems disclaimer.

If you import a version of glibc other than 2.4, please check the information there (in particular their LICENSES file) for potential inclusion of new files in the libm subdirectories that are used here.
