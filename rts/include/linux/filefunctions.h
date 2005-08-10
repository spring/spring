/*
 * findfiles.h
 * Linux file handling functions
 */
#ifndef _POSIX_FILE_H
#define _POSIX_FILE_H

#include <vector>
#include <glob.h>
#include <iostream>

static inline std::vector<std::string> find_files(std::string pattern, std::string patternpath)
{
	std::string fullpattern(patternpath+pattern);
	std::vector<std::string> found;
	glob_t *pglob = (glob_t*) malloc(sizeof(glob_t));
	int a,globret;
        if( !(globret=glob( fullpattern.c_str(), 0, NULL, pglob )) ) {
                for( a=0; a < pglob->gl_pathc; a++)
                         found.push_back( pglob->gl_pathv[a] );
        } else {
		if( globret == GLOB_NOMATCH )
			MessageBox(0,"No file matches : " + fullpattern,"find_files glob",MB_ICONEXCLAMATION);
		else
			MessageBox(0,"Other glob error","find_files glob",0);
	}
	globfree(pglob);
        free(pglob);
	return found;
}

#endif /* _POSIX_FILE_H */
