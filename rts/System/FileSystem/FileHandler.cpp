#include "StdAfx.h"
#include "FileHandler.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <boost/regex.hpp>
#include "VFSHandler.h"
#include "Platform/FileSystem.h"
#include "mmgr.h"

using namespace std;



/******************************************************************************/
/******************************************************************************/

CFileHandler::CFileHandler(const char* filename, const char* modes)
: hpiFileBuffer(NULL), hpiOffset(0), filesize(-1), ifs(NULL)
{
	Init(filename, modes);
}


CFileHandler::CFileHandler(const string& filename, const string& modes)
: hpiFileBuffer(NULL), hpiOffset(0), filesize(-1), ifs(NULL)
{
	Init(filename, modes);
}


CFileHandler::~CFileHandler(void)
{
	if (ifs) {
		delete ifs;
	}
	if (hpiFileBuffer) {
		delete[] hpiFileBuffer;
	}
}


/******************************************************************************/

bool CFileHandler::TryRawFS(const string& filename)
{
	const string rawpath = filesystem.LocateFile(filename);
	ifs = SAFE_NEW ifstream(rawpath.c_str(), ios::in | ios::binary);
	if (ifs && !ifs->bad() && ifs->is_open()) {
		ifs->seekg(0, ios_base::end);
		filesize = ifs->tellg();
		ifs->seekg(0, ios_base::beg);
		return true;
	}
	delete ifs;
	ifs = NULL;
	return false;
}


bool CFileHandler::TryModFS(const string& filename)
{
	if (hpiHandler == NULL) {
		return false;
	}

	const string file = StringToLower(filename);

	hpiLength = hpiHandler->GetFileSize(file);
	if (hpiLength != -1) {
		hpiFileBuffer = SAFE_NEW unsigned char[hpiLength];
		if (hpiHandler->LoadFile(file, hpiFileBuffer) < 0) {
			delete[] hpiFileBuffer;
			hpiFileBuffer = NULL;
		}
		else {
			filesize = hpiLength;
			return true;
		}
	}
	return false;
}


bool CFileHandler::TryMapFS(const string& filename)
{
	return TryModFS(filename); // FIXME
}


bool CFileHandler::TryBaseFS(const string& filename)
{
	return TryModFS(filename); // FIXME
}


void CFileHandler::Init(const string& filename, const string& modes)
{
	const char* c = modes.c_str();
	while (c[0] != 0) {
		if (c[0] == SPRING_VFS_RAW[0])  { if (TryRawFS(filename))  { return; } }
		if (c[0] == SPRING_VFS_MOD[0])  { if (TryModFS(filename))  { return; } }
		if (c[0] == SPRING_VFS_MAP[0])  { if (TryMapFS(filename))  { return; } }
		if (c[0] == SPRING_VFS_BASE[0]) { if (TryBaseFS(filename)) { return; } }
		c++;
	}
}


/******************************************************************************/

bool CFileHandler::FileExists() const
{
	return (ifs || hpiFileBuffer);
}


void CFileHandler::Read(void* buf,int length)
{
	if (ifs) {
		ifs->read((char*)buf, length);
	}
	else if (hpiFileBuffer) {
		if ((length + hpiOffset) > hpiLength) {
			length = hpiLength - hpiOffset;
		}
		if (length > 0) {
			memcpy(buf, &hpiFileBuffer[hpiOffset], length);
			hpiOffset += length;
		}
	}
}


void CFileHandler::Seek(int length)
{
	if (ifs) {
		ifs->seekg(length);
	} else if (hpiFileBuffer){
		hpiOffset = length;
	}
}


int CFileHandler::Peek() const
{
	if (ifs) {
		return ifs->peek();
	}
	else if (hpiFileBuffer){
		if (hpiOffset < hpiLength) {
			return hpiFileBuffer[hpiOffset];
		} else {
			return EOF;
		}
	}
	return EOF;
}


bool CFileHandler::Eof() const
{
	if (ifs) {
		return ifs->eof();
	}
	if (hpiFileBuffer) {
		return (hpiOffset >= hpiLength);
	}
	return true;
}


int CFileHandler::FileSize() const
{
   return filesize;
}


int CFileHandler::GetPos() const
{
	if (ifs) {
		return ifs->tellg();
	} else {
		return hpiOffset;
	}
}


bool CFileHandler::LoadStringData(string& data)
{
	if (!FileExists()) {
		return false;
	}
	char* buf = SAFE_NEW char[filesize];
	Read(buf, filesize);
	data.append(buf, filesize);
	delete[] buf;
	return true;	
}


/******************************************************************************/

vector<string> CFileHandler::FindFiles(const string& path, const string& pattern)
{
	vector<string> found = filesystem.FindFiles(path, pattern);
	boost::regex regexpattern(filesystem.glob_to_regex(pattern), boost::regex::icase);
	vector<string> f;

	if (hpiHandler) {
		f = hpiHandler->GetFilesInDir(path);
	}

	for (vector<string>::iterator fi = f.begin(); fi != f.end(); ++fi) {
		if (boost::regex_match(*fi, regexpattern)) {
			found.push_back(path + *fi);
		}
	}
	return found;
}


vector<string> CFileHandler::DirList(const string& path,
                                     const string& pattern,
                                     const string& modes)
{
	const string pat = pattern.empty() ? "*" : pattern;

	set<string> fileSet;
	const char* c = modes.c_str();
	while (c[0] != 0) {
		if (c[0] == SPRING_VFS_RAW[0])  { InsertRawFiles(fileSet, path, pat);  }
		if (c[0] == SPRING_VFS_MOD[0])  { InsertModFiles(fileSet, path, pat);  }
		if (c[0] == SPRING_VFS_MAP[0])  { InsertMapFiles(fileSet, path, pat);  }
		if (c[0] == SPRING_VFS_BASE[0]) { InsertBaseFiles(fileSet, path, pat); }
		c++;
	}
	vector<string> fileVec;
	set<string>::const_iterator it;
	for (it = fileSet.begin(); it != fileSet.end(); ++it) {
		fileVec.push_back(*it);
	}
	return fileVec;	
}


bool CFileHandler::InsertRawFiles(set<string>& fileSet,
                                  const string& path,
                                  const string& pattern)
{
	boost::regex regexpattern(filesystem.glob_to_regex(pattern), boost::regex::icase);

	vector<string> found = filesystem.FindFiles(path, pattern);
	vector<string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		if (boost::regex_match(*fi, regexpattern)) {
			fileSet.insert(fi->c_str());
		}
	}

	return true;
}


bool CFileHandler::InsertModFiles(set<string>& fileSet,
                                  const string& path,
                                  const string& pattern)
{
	if (!hpiHandler) {
		return false;
	}

	string prefix = path;
	if (path.find_last_of("\\/") != (path.size() - 1)) {
		prefix += '/';
	}

	boost::regex regexpattern(filesystem.glob_to_regex(pattern), boost::regex::icase);

	vector<string> found = hpiHandler->GetFilesInDir(path);
	vector<string>::const_iterator fi;
	for (fi = found.begin(); fi != found.end(); ++fi) {
		if (boost::regex_match(*fi, regexpattern)) {
			fileSet.insert(prefix + *fi);
		}
	}

	return true;
}


bool CFileHandler::InsertMapFiles(set<string>& fileSet,
                                  const string& path,
                                  const string& pattern)
{
	return InsertModFiles(fileSet, path, pattern); // FIXME
}


bool CFileHandler::InsertBaseFiles(set<string>& fileSet,
                                   const string& path,
                                   const string& pattern)
{
	return InsertModFiles(fileSet, path, pattern); // FIXME
}


string CFileHandler::AllowModes(const string& modes, const string& allowed)
{
	string newModes;
	for (int i = 0; i < modes.size(); i++) {
		if (allowed.find(modes[i]) != string::npos) {
			newModes += modes[i];
		}
	}
	return newModes;
}


string CFileHandler::ForbidModes(const string& modes, const string& forbidden)
{
	string newModes;
	for (int i = 0; i < modes.size(); i++) {
		if (forbidden.find(modes[i]) == string::npos) {
			newModes += modes[i];
		}
	}
	return newModes;
}


/******************************************************************************/
/******************************************************************************/
