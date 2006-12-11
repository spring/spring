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

	// Non-empty directories to look in should have a trailing backslash
	if (!dir.empty()) {
		if (dir[dir.length() - 1] != '/')
			dir += "/";
	}

	// Possible optimization: find the start iterator by inserting the dir and see where it ends up
	bool foundMatch = false;
	//This breaks VC8, specificaly boost iterator assertions
	//for (map<string, FileData>::iterator i = files.begin(); i != files.end(); ++i) {
	
	map<string, FileData>::iterator filesStart = files.begin();
	map<string, FileData>::iterator filesEnd = files.end();
	while (filesStart != filesEnd)
		{
		//This breaks VC8, specificaly boost iterator assertions
		//if (equal(dir.begin(), dir.end(), i->first.begin())) {
		int thisLength = filesStart->first.length();
		string path = filesystem.GetDirectory(filesStart->first);
		// Test to see if this file startwith the dir path
		if (dir.compare(path) == 0)
			{

			// Strip pathname
			string name = filesStart->first.substr(dir.length());

			// Do not return files in subfolders
			if (name.find('\\') == string::npos && name.find('/') == string::npos)
				ret.push_back(name);

			foundMatch = true;
			}
		else {
			// If we have had a match previously but this one isn't a match, there will be no more matches
			if (foundMatch)
				break;
			}
		filesStart++;
		}

	return ret;
}
