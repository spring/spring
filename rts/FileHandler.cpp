#include "StdAfx.h"
#include "FileHandler.h"
#include <fstream>
//#include "HpiHandler.h"
#include "VFSHandler.h"
#include <algorithm>
#include <cctype>
//#include "mmgr.h"
#include "filefunctions.h"

using namespace std;

CFileHandler::CFileHandler(const char* filename)
: hpiFileBuffer(0),
	hpiOffset(0),
	filesize(-1)
{
	Init(filename);
}

CFileHandler::CFileHandler(std::string filename)
: hpiFileBuffer(0),
	hpiOffset(0),
	filesize(-1)
{
	Init(filename.c_str());
}

void CFileHandler::Init(const char* filename)
{
        ifs=new std::ifstream(filename, ios::in|ios::binary);
	if(ifs->is_open())
	{
		ifs->seekg(0, ios_base::end);
		filesize = ifs->tellg();
		ifs->seekg(0, ios_base::beg);
		return;
	}
	delete ifs;
	ifs=0;

	if(!hpiHandler)
		return;

	//hpi dont have info about directory
	std::string file=filename;
	std::transform(file.begin(), file.end(), file.begin(), (int (*)(int))std::tolower);

	hpiLength=hpiHandler->GetFileSize(file);
	if(hpiLength!=-1){
		hpiFileBuffer=new unsigned char[hpiLength];
		hpiHandler->LoadFile(file,hpiFileBuffer);
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

std::vector<std::string> CFileHandler::FindFiles(std::string pattern)
{
	std::vector<std::string> found;
	std::string patternPath=pattern;
	if(patternPath.find_last_of('\\')!=string::npos){
		patternPath.erase(patternPath.find_last_of('\\')+1);
	}
	if(patternPath.find_last_of('/')!=string::npos){
		patternPath.erase(patternPath.find_last_of('/')+1);
	}
	if(pattern.find('\\')==string::npos && pattern.find('/')==string::npos)
		patternPath.clear();

	std::string filter=pattern;
	filter.erase(0,patternPath.length());
	found = find_files(filter,patternPath);

	//todo: get a real regex handler
	while(filter.find_last_of('*')!=string::npos)
		filter.erase(filter.find_last_of('*'),1);
//	while(filter.find_last_of('.')!=string::npos)
//		filter.erase(filter.find_last_of('.'),1);
	std::transform(filter.begin(), filter.end(), filter.begin(), (int (*)(int))std::tolower);

	std::vector<std::string> f;
	if(hpiHandler)
		f=hpiHandler->GetFilesInDir(patternPath);

	for(std::vector<std::string>::iterator fi=f.begin();fi!=f.end();fi++){
		std::transform(fi->begin(), fi->end(), fi->begin(), (int (*)(int))std::tolower);
		int a=fi->find(filter);
		if(filter.empty() || a==fi->length()-filter.length()){
			found.push_back(patternPath+*fi);
		}
	}

	return found;
}

int CFileHandler::FileSize()
{
   return filesize;
}
