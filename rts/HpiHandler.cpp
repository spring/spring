// HpiHandler.cpp: implementation of the CHpiHandler class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "HpiHandler.h"
#include <windows.h>
#include "HPIUtil.h"
#include "myGL.h"
#include "RegHandler.h"
#include "InfoConsole.h"
#include "filefunctions.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CHpiHandler* hpiHandler=0;

CHpiHandler::CHpiHandler()
{
	string taDir;

//	if(taDir[taDir.size()-1]!='\\')
//		taDir+="\\";

	taDir="./";
	char t[500];
	sprintf(t,"Mapping hpi files in %s",taDir.c_str());
	PrintLoadMsg(t);

	FindHpiFiles("*.gp4",taDir);
	FindHpiFiles("*.swx",taDir);
	FindHpiFiles("*.gp3",taDir);
	FindHpiFiles("*.ufo",taDir);
	FindHpiFiles("*.ccx",taDir);
	FindHpiFiles("*.hpi",taDir);

	// If no hpi files are found, spring is probably running without any ta content
	// In that case, enable the spanktower!
	useBackupUnit = false;
	if (files.size() == 0) {
		info->AddLine("No hpi files found, enabling backup unit");
		FindHpiFiles("*.sdu",taDir);
		useBackupUnit = true;
	}
}

CHpiHandler::~CHpiHandler()
{
}

int CHpiHandler::GetFileSize(string name)
{
	MakeLower(name);
	if(files.find(name)==files.end())
		return -1;
	return files[name].size;
}

int CHpiHandler::LoadFile(string name, void *buffer)
{
	MakeLower(name);
	if(files.find(name)==files.end())
		return 0;
	_HPIFILE* hpi=(_HPIFILE*)HPIOpen((char*)files[name].hpiname.c_str());
	char* file=HPIOpenFile(hpi, (char*)name.c_str());
//	char* file= const_cast<char*> (HPIOpenFile(hpi, name.c_str()).c_str());
	if (file)
	{	// avoid crash if file is missing.
		HPIGet((char*)buffer,file,0,files[name].size);
		HPICloseFile(file);
	}

	HPIClose(hpi);
	return files[name].size;
}

void CHpiHandler::FindHpiFiles(string pattern,string path)
{
	std::vector<std::string> found = find_files(pattern,path);
	for (std::vector<std::string>::iterator it = found.begin(); it!=found.end(); it++)
		SearchHpiFile((char*)it->c_str());
}

void CHpiHandler::SearchHpiFile(char* name)
{
	_HPIFILE* hpi=(_HPIFILE*)HPIOpen(name);
	if(hpi==0)
		return;
	char file[512];
	int type;
	int size;

//	MessageBox(0,name,"Test",0);
	bool nextFile=HPIGetFiles(hpi, 0, file, &type, &size);

	while(nextFile!=0){
		if(type==0){
			RegisterFile(name,file,size);
//			info->AddLine(file);
		}
		nextFile=HPIGetFiles(hpi, nextFile, file, &type, &size);
	}
	HPIClose(hpi);
}

void CHpiHandler::RegisterFile(char *hpi, char *name, int size)
{
	MakeLower(name);
	if(files.find(name)==files.end()){
		FileData fd;
		fd.hpiname=hpi;
		fd.size=size;
		files[name]=fd;
	}
}


void CHpiHandler::MakeLower(string &s)
{
	for(unsigned int a=0;a<s.size();++a){
		if(s[a]>='A' && s[a]<='Z')
			s[a]=s[a]+'a'-'A';
	}
}

void CHpiHandler::MakeLower(char *s)
{
	for(int a=0;s[a]!=0;++a)
		if(s[a]>='A' && s[a]<='Z')
			s[a]=s[a]+'a'-'A';
}

std::vector<std::string> CHpiHandler::GetFilesInDir(std::string dir)
{
	std::vector<std::string> found;

	string taDir="./";
	FindHpiFilesForDir("*.gp4",taDir,dir,found);
	FindHpiFilesForDir("*.swx",taDir,dir,found);
	FindHpiFilesForDir("*.gp3",taDir,dir,found);
	FindHpiFilesForDir("*.ufo",taDir,dir,found);
	FindHpiFilesForDir("*.ccx",taDir,dir,found);
	FindHpiFilesForDir("*.hpi",taDir,dir,found);

	if (useBackupUnit) 
		FindHpiFilesForDir("*.sdu",taDir,dir,found);

	return found;
}

void CHpiHandler::FindHpiFilesForDir(string pattern,string path,string subPath,std::vector<std::string>& found)
{
	std::vector<std::string> result = find_files(pattern,path);
	for (std::vector<std::string>::iterator it = result.begin(); it != result.end(); it++)
		SearchHpiFileInDir((char*)it->c_str(),subPath,found);
}

void CHpiHandler::SearchHpiFileInDir(char* name,string subPath,std::vector<std::string>& found)
{
	_HPIFILE* hpi=(_HPIFILE*)HPIOpen(name);
	if(hpi==0)
		return;
	char file[512];
	int type;
	int size;

//	MessageBox(0,name,"Test",0);
	char path[500];
	subPath.erase(subPath.length()-1);
	strcpy(path,subPath.c_str());
	bool nextFile=HPIDir(hpi, 0, path, file, &type, &size);

	while(nextFile!=0){
		if(type==0)
			found.push_back(file);
		nextFile=HPIDir(hpi, nextFile, path, file, &type, &size);
	}
	HPIClose(hpi);
}

