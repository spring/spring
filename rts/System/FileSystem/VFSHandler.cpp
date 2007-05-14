#include "StdAfx.h"
#include "VFSHandler.h"
#ifdef _WIN32
#include <io.h>
#endif
#include "ArchiveFactory.h"
#include <algorithm>
#include "Rendering/GL/myGL.h"
#include "Platform/FileSystem.h"
#include "mmgr.h"

CVFSHandler* hpiHandler=0;

CVFSHandler::CVFSHandler(bool mapArchives)
{
	if (mapArchives)
		MapArchives("./");
}

void CVFSHandler::MapArchives(const string& taDir)
{
	FindArchives("*.gp4", taDir);
	FindArchives("*.swx", taDir);
	FindArchives("*.gp3", taDir);
	FindArchives("*.ccx", taDir);
	FindArchives("*.ufo", taDir);
	FindArchives("*.hpi", taDir);
	FindArchives("*.sdz", taDir);
	FindArchives("*.sd7", taDir);
	FindArchives("*.sdd", taDir);
}

void CVFSHandler::FindArchives(const string& pattern, const string& path)
{
	std::vector<std::string> found = filesystem.FindFiles(path, pattern);
	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); ++it)
		AddArchive(it->c_str(), false);
}

// Override determines whether if conflicts overwrites an existing entry in the virtual filesystem or not
bool CVFSHandler::AddArchive(string arName, bool override)
{
//	StringToLowerInPlace(arName);

	CArchiveBase* ar = archives[arName];
	if (!ar) {
		ar = CArchiveFactory::OpenArchive(arName);
		if (!ar)
			return false;
		archives[arName] = ar;
	}

	int cur;
	string name;
	int size;

	for (cur = ar->FindFiles(0, &name, &size); cur != 0; cur = ar->FindFiles(cur, &name, &size)) {
		StringToLowerInPlace(name);
		if ((!override) && (files.find(name) != files.end())) 
			continue;

		FileData d;
		d.ar = ar;
		d.size = size;
		files[name] = d;
	}
	return true;
}

CVFSHandler::~CVFSHandler(void)
{
	for (map<string, CArchiveBase*>::iterator i = archives.begin(); i != archives.end(); ++i) {
		delete i->second;
	}
}

int CVFSHandler::LoadFile(string name, void* buffer)
{
	StringToLowerInPlace(name);
	filesystem.ForwardSlashes(name);
	FileData fd = files[name];

	int fh = fd.ar->OpenFile(name);
	if (!fh)
		return -1;
	fd.ar->ReadFile(fh, buffer, fd.size);
	fd.ar->CloseFile(fh);

	return fd.size;
}

int CVFSHandler::GetFileSize(string name)
{
	StringToLowerInPlace(name);
	filesystem.ForwardSlashes(name);
	map<string, FileData>::iterator f = files.find(name);

	if (f == files.end())
		return -1;
	else
		return f->second.size;
}

// Returns all the files in the given (virtual) directory without the preceeding pathname
vector<string> CVFSHandler::GetFilesInDir(string dir)
{
	vector<string> ret;
	StringToLowerInPlace(dir);
	filesystem.ForwardSlashes(dir);

	map<string, FileData>::const_iterator filesStart = files.begin();
	map<string, FileData>::const_iterator filesEnd   = files.end();

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		string::size_type dirLast = (dir.length() - 1);
		if (dir[dirLast] != '/') {
			dir += "/";
			dirLast++;
		}
		// limit the iterator range
		string dirEnd = dir;
		dirEnd[dirLast] = dirEnd[dirLast] + 1;
		filesStart = files.lower_bound(dir);
		filesEnd   = files.upper_bound(dirEnd);
	}
	
	while (filesStart != filesEnd) {
		const string path = filesystem.GetDirectory(filesStart->first);

		// Test to see if this file start with the dir path
		if (path.compare(0, dir.length(), dir) == 0) {

			// Strip pathname
			const string name = filesStart->first.substr(dir.length());

			// Do not return files in subfolders
			if ((name.find('/') == string::npos) &&
			    (name.find('\\') == string::npos)) {
				ret.push_back(name);
			}
		}
		filesStart++;
	}

	return ret;
}
