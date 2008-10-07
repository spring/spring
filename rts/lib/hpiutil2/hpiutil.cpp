/*
 * hpiutil.cpp
 * HPIUtil main implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 *
 * This file is part of hpiutil2.
 *
 * hpiutil2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * hpiutil2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hpiutil2; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "hpiutil.h"

#include <string.h>

#ifdef _MSC_VER
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif

/*
 * HPIOpen
 * Opens a given hpi file and returns a pointer to its hpifile object
 * filename - path to hpi file
 */
hpiutil::hpifile* hpiutil::HPIOpen(const char *filename)
{
	hpifile *hpi = new hpifile(filename);
	if (hpi->valid)
		return hpi;
	else {
    delete hpi;
		return NULL;
  }
}

/*
 * HPIClose
 * Closes an hpi file
 * hpi - hpifile object to close
 */
void hpiutil::HPIClose(hpifile &hpi)
{
	delete &hpi;
}

/*
 * HPIReadFlatList
 * searches an hpi file's flatlist for a given entry, should not be necessary to call manually
 * returns pointer to hpientry of this object if found
 * hpi - hpi file to search
 * name - path of file to search for
 * dir - whether to search directories or files
 */
hpiutil::hpientry_ptr hpiutil::HPIReadFlatList(hpifile const &hpi, const char *name, const bool dir)
{
	std::vector<hpientry_ptr> const& tmp = hpi.flatlist;
	int len = strlen(name);

	/*
	 * Make an alternate name in case of platform differences
	 */
	char *altname = (char*)calloc(len+1,sizeof(char));
	for (int i = 0; i < len; i++) {
		if (name[i] == PATHSEPARATOR)
			altname[i] = OTHERPATHSEPARATOR;
		else
			altname[i] = name[i];
	}
	altname[len] = '\0';

	for (std::vector<hpientry_ptr>::const_iterator it = tmp.begin(); it != tmp.end(); it++) {
		if ((!STRCASECMP((*it)->path().c_str(),name) || !STRCASECMP((*it)->path().c_str(),altname)) && ((*it)->directory == dir)) {
			free(altname);
			return *it;
		}
	}
	free(altname);
	return hpientry_ptr();
}

/*
 * HPIOpenFile
 * searches for a file inside an hpi
 * returns a pointer to the hpientry if found
 * hpi - hpi file to search
 * filename - name of file to find
 */
hpiutil::hpientry_ptr hpiutil::HPIOpenFile(hpifile const &hpi, const char *filename)
{
	return HPIReadFlatList(hpi,filename,false);
}

/*
 * HPICloseFile
 * closes an hpientry
 * he - hpientry to close
 */
void hpiutil::HPICloseFile(hpientry_ptr &he)
{
  // no need since shared_ptr removes the object!
	/*
	 * Should really only be deleted if you never intend
	 * to read the file again
	 */
//	delete &he;
}

/*
 * HPIGet
 * loads a file's data into a buffer
 * returns the number of bytes read
 * dest - buffer to load into
 * he - hpientry representing file to load
 * offset - offset in hpi file (unused, for compatibility only)
 * bytecount - length of file to load (unused, for compatibility only)
 */
boost::uint32_t hpiutil::HPIGet(char *dest, hpientry_ptr const &he, const int offset, const int bytecount)
{
	return he->file->getdata(he,(boost::uint8_t*)dest);
}

/*
 * HPIGetFiles
 * returns a listing of all the files in an hpi
 * as a vector of hpientry pointers
 * hpi - hpi file to list
 */
std::vector<hpiutil::hpientry_ptr> hpiutil::HPIGetFiles(hpifile const &hpi)
{
	return hpi.flatlist;
}

/*
 * HPIDir
 * gets the contents of a directory in an hpi file
 * returns a vector of hpientry pointers
 * hpi - hpi file to use
 * dirname - name of directory to list
 */
std::vector<hpiutil::hpientry_ptr> hpiutil::HPIDir(hpifile const &hpi,const char *dirname)
{
	hpientry_ptr dir = HPIReadFlatList(hpi,dirname,true);
	if (dir)
		return dir->subdir;
	else
		return std::vector<hpientry_ptr>();
}
