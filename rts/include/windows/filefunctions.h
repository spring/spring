/*
 * findfiles.h
 * Windows file handling functions
 */
#ifndef _WIN32_FILE_H
#define _WIN32_FILE_H

#include <io.h>
#include <vector>

static inline std::vector<std::string> find_files(std::string pattern, std::string patternpath)
{
	std::vector<std::string> found;
	struct _finddata_t files;    
	long hFile;
	int morefiles=0;

	if( (hFile = _findfirst( pattern.c_str(), &files )) == -1L ){
		morefiles=-1;
	}

	int numfiles=0;
	while(morefiles==0){
		
		found.push_back(patternpath+files.name);
		morefiles=_findnext( hFile, &files ); 
	}
	return found;

}

#endif /* _WIN32_FILE_H */
