/*
 * findfiles.h
 * Linux file handling functions
 */
#ifndef _POSIX_FILE_H
#define _POSIX_FILE_H

#include <vector>
#include <glob.h>
#include <ctype.h>
#include <iostream>
#include <windows.h>
#include <dirent.h>
#include <sys/stat.h>

static inline std::vector<std::string> find_files(std::string pattern, std::string patternpath, const bool recurse = true)
{
	std::string fullpattern(patternpath+pattern);
	std::string regpattern;
	for (int i = 0; i < fullpattern.length(); i++) {
		char ch = fullpattern.at(i);
		if (isalpha(ch)) {
			char tmpstr[4];
			sprintf(tmpstr,"[%c%c]",toupper(ch),tolower(ch));
			regpattern.append(tmpstr);
		} else
			regpattern.append(1,ch);
	}
	std::vector<std::string> found;
	glob_t *pglob = (glob_t*) malloc(sizeof(glob_t));
	int a,globret;
        if( !(globret=glob( regpattern.c_str(), 0, NULL, pglob )) ) {
                for( a=0; a < pglob->gl_pathc; a++)
                         found.push_back( pglob->gl_pathv[a] );
        } else {
#ifdef DEBUG
		if( globret == GLOB_NOMATCH )
			MessageBox(0,"No file matches : " + fullpattern,"find_files glob",MB_ICONEXCLAMATION);
		else
			MessageBox(0,"Other glob error","find_files glob",0);
#endif
	}
	globfree(pglob);
        free(pglob);
	if (recurse) {
		struct dirent **namelist;
		int n = scandir(patternpath.c_str(),&namelist,0,alphasort);
		if (n<0) {
#ifdef DEBUG
			MessageBox(0,"Could not read dir " + patternpath,"find_files scandir",0);
#endif
		} else {
			while (n--) {
				namelist[n]->d_name;
				if (strcmp(namelist[n]->d_name,".")&&strcmp(namelist[n]->d_name,"..")) {
					struct stat *st = (struct stat*)malloc(sizeof(struct stat));
					stat((patternpath+std::string(namelist[n]->d_name)).c_str(),st);
					if (S_ISDIR(st->st_mode)) {
						std::vector<std::string> subret = find_files(pattern,(patternpath+std::string(namelist[n]->d_name)+std::string("/")).c_str(),true);
						if (!subret.empty()) {
							for (std::vector<std::string>::iterator it=subret.begin(); it != subret.end(); it++) {
								found.push_back(*it);
							}
						}
					}
					free(st);
				}
				free(namelist[n]);
			}
			free(namelist);
		}
	}
#ifdef DEBUG
	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); it++) {
		if (recurse) {
			printf("Recursive glob %s in %s returned: %s\n",pattern.c_str(),patternpath.c_str(),it->c_str());
		} else {
			printf("Glob %s in %s returned: %s\n",pattern.c_str(),patternpath.c_str(),it->c_str());
		}
	}
#endif
	return found;
}

#endif /* _POSIX_FILE_H */
