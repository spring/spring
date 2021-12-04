/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// # Header includes directory change tool
// Used to automatically change #include "[somepath/]header" to #include "correctheaderpath"
// Give the base project directory as argument:
// Example for spring:
// 	dch <trunk-path>/rts -ISystem

#include <cstdio>
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <set>
#include <list>
#include <map>

#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

// define a common indicator for 32bit or 64bit-ness
#if defined _WIN64 || defined __LP64__ || defined __ppc64__ || defined __ILP64__ || defined __SILP64__ || defined __LLP64__ || defined(__sparcv9)
#define __arch64__
#else
#define __arch32__
#endif

// define a cross-platform/-compiler compatible "%z" format replacement for
// printf() style functions.
// "%z" being the propper way for size_t typed values,
// but support for it has not yet spread wide enough.
#if defined __arch64__
#define __SIZE_T_PRINTF_FORMAT__ "%lu"
#else
#define __SIZE_T_PRINTF_FORMAT__ "%u"
#endif
// a shorter form
#define _STPF_ __SIZE_T_PRINTF_FORMAT__


using std::vector;
using std::list;
using std::map;
using std::set;
using std::string;


struct file_t {
	string path;
	string name;
	string ext;
	string inclpath; // path used to include this file
};

list<file_t> filelist;


void RemoveExtension(char *file)
{
	int a = strlen(file)-1;
	while(a>0)
	{
		if(file[a] == '.') {
			file[a] = 0;
			break;
		}
		a--;
	}
}

const char *GetExtension(const char *file)
{
	int a = strlen(file)-1;
	while(a>0)
	{
		if(file[a] == '.')
			return &file[a+1];
		a--;
	}
	return 0;
}


const char *GetFilename (const char *file)
{
	int a = strlen (file) - 1;
	while (a>0)
	{
		if(file[a] == '/' || file[a] == '\\')
			return &file[a+1];
		a--;
	}
	return file;
}

#ifdef WIN32
void Scan(const string& curPath, const string& storePath)
{
	struct _finddata_t files;
	intptr_t handle;
	int moreFiles;

	string curPattern = curPath;
	curPattern += "/*.*";

	handle = _findfirst(curPattern.c_str(), &files);
	for (moreFiles = (int)handle; moreFiles != -1; moreFiles = _findnext(handle, &files)) {

		string filepath=curPath + "/" + files.name;
		//printf("looking at %s\n", filepath.c_str());

		// Is it a directory?
		if ((files.attrib & 16) != 0) {

			// Avoid the special directories
			if (files.name[0] != '.') {
				string newPath = curPath + "/" + files.name;
				string newStorePath;
				if (storePath.empty()) newStorePath=files.name;
				else newStorePath = storePath + "/" + files.name;
				Scan(newPath, newStorePath);
			}
		} else {
			file_t f;
			f.name = files.name;
			f.path = filepath;
			if(storePath.empty())
				f.inclpath = files.name;
			else
				f.inclpath = storePath + "/" +  files.name;
			const char *ext=GetExtension (files.name);
			if (ext)
				f.ext = ext;

			filelist.push_back (f);
		}
	}
	_findclose(handle);
}
#else
void Scan(const string& curPath, const string& storePath)
{
	DIR *dp;
	struct dirent *ep;

	dp = opendir(curPath.c_str());
	if (dp == NULL) {
		perror((string("opendir: ") + curPath).c_str());
		return;
	}

	while( (ep = readdir(dp)) != NULL ) {

		string filepath = curPath + "/" + ep->d_name;
		struct stat info;

		if (stat(filepath.c_str(), &info) != 0) {
			perror((string("stat: ") + filepath).c_str());
			continue;
		}

		// Is it a directory?
		if (S_ISDIR(info.st_mode)) {

			// Avoid the special directories
			if (ep->d_name[0] != '.') {
				string newPath = filepath;
				string newStorePath;
				if (storePath.empty()) newStorePath=ep->d_name;
				else newStorePath = storePath + "/" + ep->d_name;
				Scan(newPath, newStorePath);
			}
		} else {
			file_t f;
			f.name = ep->d_name;
			f.path = filepath;
			if(storePath.empty())
				f.inclpath = ep->d_name;
			else
				f.inclpath = storePath + "/" +  ep->d_name;
			const char *ext=GetExtension (ep->d_name);
			if (ext)
				f.ext = ext;

			filelist.push_back (f);
		}
	}

	closedir(dp);
}
#endif

void SimplifyFilePaths(const set<string>& inclPaths)
{
	for (list<file_t>::iterator fi = filelist.begin(); fi != filelist.end(); ++fi) {
		int p=0;
		string path, endInclPath;

		while (p < fi->inclpath.length ()) {
			while (fi->inclpath[p] != '/' && p < fi->inclpath.length()) {
				p ++;
			}

			if (p == fi->inclpath.length()) // go to next file?
				break;

			p++;
			path=fi->inclpath.substr (0, p);
			if (inclPaths.find (path) != inclPaths.end())
				endInclPath = fi->inclpath.substr (p,fi->inclpath.length());
		}

		if (!endInclPath.empty())
			fi->inclpath = endInclPath;
	}
}

string GetHeaderPath (const string& headerPath, const string& srcFilePath)
{
	// only set the part of the paths that is not shared
	int i=0;

	int lastSharedPos=0;

	while (i < headerPath.length() && i < srcFilePath.length()) {
		if (headerPath[i] != srcFilePath[i])
			break;

		if (headerPath[i]=='/')
			lastSharedPos=i+1;
		i++;
	}

	int srcFilename=srcFilePath.rfind ('/');
	if (srcFilename!=string::npos) {
		srcFilename++;
		if (srcFilename!=lastSharedPos)
			lastSharedPos=0;
	}

	return headerPath.substr (lastSharedPos, headerPath.length());
}

int main (int argc, char *argv[])
{
	const char *searchPath=0;
	set<string> inclPaths;  // Include paths defined with -I
	map<string, file_t*> hmap; // all headers

	for (int a=1;a<argc;a++) {
		const char *arg=argv[a];
		if (arg[0]=='-' && arg[1]=='I') {
			string path=&arg[2];
			if(!path.empty()) {
				if(path[path.length ()-1] != '/')
					path+='/';
				inclPaths.insert (path);
			}
		} else
			searchPath=arg;
	}

	if (!searchPath) {
		printf ("Usage example: dch c:\\projects\\someproj -\n");
		return -1;
	}

	Scan(searchPath,string());

	if (filelist.empty())
		return 0;

	// strip away unnecessary parts based on the added include paths
	SimplifyFilePaths (inclPaths);

	printf ("Creating header list...");
	for (list<file_t>::iterator i=filelist.begin();i!=filelist.end();++i)
	{
		if (i->ext == "h")  {
			printf("Header: %s\n", i->inclpath.c_str());

			hmap [i->name]=&*i;
		}
	}
	printf (_STPF_ " headers.\n", hmap.size());

	for (list<file_t>::iterator i=filelist.begin();i!=filelist.end();++i)
	{
		// read all cpp and h files for #include <headerfile>
		// and replace the headerfile with header path

		if (i->ext != "cpp" && i->ext != "h")
			continue;

		FILE *f=fopen(i->path.c_str(), "rb");
		if (!f) continue;

		fseek (f,0,SEEK_END);
		int len=ftell(f);
		fseek (f,0,SEEK_SET);

		vector <char> buffer;
		buffer.resize(len);
		fread (&buffer.front(), len, 1, f);
		fclose (f);

		string result;

		int x=0;
		const char *incl="#include \"";
		int a;
		while (x < buffer.size()) {
			char *src = &buffer[x];
			for (a=0;incl[a] && src[a]==incl[a];a++)
				;

			if (!incl[a]) {
				// find the header name
				char *headernamesrc = &src[a];
				string headerpath;
				int j=0;

				while (headernamesrc[j]!='"' && headernamesrc+j<&buffer.back())
					headerpath += headernamesrc[j++];

				// strip the directory part of it
				string headername = GetFilename (headerpath.c_str());

				// match headername with a registered header
				map<string,file_t*>::iterator fh=hmap.find (headername);
				if (fh == hmap.end()) {
					result += incl;
					result += headername;
					x+=j+a;
					continue;
				}

				result += incl;
				result += GetHeaderPath (fh->second->inclpath, i->inclpath);
			//	printf ("In file %s:  Replaced %s with %s\n", i->path.c_str(), headername.c_str(), fh->second->localpath.c_str());

				x+=j+a;
				continue;
			}

			result += buffer[x];
			x ++;
		}

		// write the result
		string outfn=i->path;
		FILE *outf = fopen (outfn.c_str(), "wb");
		if (outf) {
			fwrite (result.c_str(),result.size(),1,outf);
			fclose (outf);
		}

		printf ("Output: %s\n", outfn.c_str());
	}

	return 0;
}

