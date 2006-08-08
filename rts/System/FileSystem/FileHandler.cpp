#include "StdAfx.h"
#include "FileHandler.h"
#include <fstream>
#include "VFSHandler.h"
#include <algorithm>
#include <cctype>
#include "Platform/FileSystem.h"
#include <boost/regex.hpp>
#include "mmgr.h"

using namespace std;

CFileHandler::CFileHandler(const char* filename)
: hpiFileBuffer(0),
	hpiOffset(0),
	filesize(-1),
	ifs(0)
{
	Init(filename);
}

CFileHandler::CFileHandler(std::string filename)
: hpiFileBuffer(0),
	hpiOffset(0),
	filesize(-1),
	ifs(0)
{
	Init(filename.c_str());
}

void CFileHandler::Init(const char* filename)
{
	ifs=filesystem.ifstream(filename, ios::in|ios::binary);
	if (ifs) {
		ifs->seekg(0, ios_base::end);
		filesize = ifs->tellg();
		ifs->seekg(0, ios_base::beg);
		return;
	}
	delete ifs;
	ifs = 0;

	if(!hpiHandler)
		return;

	//hpi dont have info about directory
	std::string file = StringToLower(filename);

	hpiLength=hpiHandler->GetFileSize(file);
	if(hpiLength!=-1){
		hpiFileBuffer=new unsigned char[hpiLength];
		if (hpiHandler->LoadFile(file,hpiFileBuffer) < 0) {
			delete[] hpiFileBuffer;
			hpiFileBuffer = 0;
		}
		else
			filesize = hpiLength;
	}
}

CFileHandler::~CFileHandler(void)
{
	if(ifs)
		delete ifs;

	if(hpiFileBuffer)
		delete[] hpiFileBuffer;
}

bool CFileHandler::FileExists()
{
	return (ifs || hpiFileBuffer);
}

void CFileHandler::Read(void* buf,int length)
{
	if(ifs){
		ifs->read((char*)buf,length);
	} else if(hpiFileBuffer){
		if(length+hpiOffset>hpiLength)
			length=hpiLength-hpiOffset;
		if(length>0){
			memcpy(buf,&hpiFileBuffer[hpiOffset],length);
			hpiOffset+=length;
		}
	}
}

void CFileHandler::Seek(int length)
{
	if(ifs){
		ifs->seekg(length);
	} else if(hpiFileBuffer){
		hpiOffset=length;
	}
}

int CFileHandler::Peek()
{
	if(ifs){
		return ifs->peek();
	} else if(hpiFileBuffer){
		if(hpiOffset<hpiLength)
			return hpiFileBuffer[hpiOffset];
		else
			return EOF;
	}
	return EOF;
}

bool CFileHandler::Eof()
{
	if(ifs){
		return ifs->eof();
	} else if(hpiFileBuffer){
		return hpiOffset>=hpiLength;
	}
	return true;
}

std::vector<std::string> CFileHandler::FindFiles(const std::string& path, const std::string& pattern)
{
	std::vector<std::string> found = filesystem.FindFiles(path, pattern);
	boost::regex regexpattern(filesystem.glob_to_regex(pattern), boost::regex::icase);
	std::vector<std::string> f;

	if(hpiHandler)
		f = hpiHandler->GetFilesInDir(path);

	for(std::vector<std::string>::iterator fi = f.begin(); fi != f.end(); ++fi){
		if (boost::regex_match(*fi, regexpattern)) {
			found.push_back(path + *fi);
		}
	}
	return found;
}

int CFileHandler::FileSize()
{
   return filesize;
}

int CFileHandler::GetPos()
{
	if(ifs)
		return ifs->tellg();
	else
		return hpiOffset;
}

