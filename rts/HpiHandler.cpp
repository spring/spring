// HpiHandler.cpp: implementation of the CHpiHandler class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "HpiHandler.h"
#include <windows.h>
#ifndef NO_IO
#include <io.h>
#endif
#include "HPIUtil.h"
#include "myGL.h"
#include "RegHandler.h"
#include "InfoConsole.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CHpiHandler* hpiHandler=0;

CHpiHandler::CHpiHandler()
{
#ifndef NO_DLL
	if((m_hDLL=LoadLibrary("hpiutil.dll"))==0)
		MessageBox(0,"Failed to find hpiutil.dll","",0);

	HPIOpen=	(LPVOID (WINAPI *)(const char*))GetProcAddress(m_hDLL,"HPIOpen");
	HPIGetFiles=(LRESULT (WINAPI *)(void *, int, LPSTR, LPINT, LPINT))GetProcAddress(m_hDLL,"HPIGetFiles");
	HPIClose=(LRESULT (WINAPI *)(void *))GetProcAddress(m_hDLL,"HPIClose");
	HPIOpenFile=(LPSTR (WINAPI *)(void *hpi, const char*))GetProcAddress(m_hDLL,"HPIOpenFile");
	HPIGet=(void (WINAPI *)(void *Dest, void *, int, int))GetProcAddress(m_hDLL,"HPIGet");
	HPICloseFile=(LRESULT (WINAPI *)(LPSTR))GetProcAddress(m_hDLL,"HPICloseFile");
	HPIDir=(LRESULT (WINAPI*)(void *hpi, int Next, LPSTR DirName, LPSTR Name, LPINT Type, LPINT Size))GetProcAddress(m_hDLL,"HPIDir");
#endif
	string taDir;

//	if(taDir[taDir.size()-1]!='\\')
//		taDir+="\\";

	taDir=".\\";
	char t[500];
	sprintf(t,"Mapping hpi files in %s",taDir.c_str());
	PrintLoadMsg(t);

	FindHpiFiles(taDir+"*.gp4",taDir);
	FindHpiFiles(taDir+"*.swx",taDir);
	FindHpiFiles(taDir+"*.gp3",taDir);
	FindHpiFiles(taDir+"*.ufo",taDir);
	FindHpiFiles(taDir+"*.ccx",taDir);
	FindHpiFiles(taDir+"*.hpi",taDir);

	// If no hpi files are found, spring is probably running without any ta content
	// In that case, enable the spanktower!
	useBackupUnit = false;
	if (files.size() == 0) {
		info->AddLine("No hpi files found, enabling backup unit");
		FindHpiFiles(taDir+"*.sdu",taDir);
		useBackupUnit = true;
	}
}

CHpiHandler::~CHpiHandler()
{
	FreeLibrary(m_hDLL);
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
	void* hpi=HPIOpen(files[name].hpiname.c_str());
#ifndef NO_WINSTUFF
	char* file=HPIOpenFile(hpi, name.c_str());
#else
	char* file= const_cast<char*> (HPIOpenFile(hpi, name.c_str()).c_str());
#endif
	if (file)
	{	// avoid crash if file is missing.
		HPIGet(buffer,file,0,files[name].size);
		HPICloseFile(file);
	}

	HPIClose(hpi);
	return files[name].size;
}

void CHpiHandler::FindHpiFiles(string pattern,string path)
{
#ifndef NO_IO
	struct _finddata_t files;    
	long hFile;
	int morefiles=0;

//	MessageBox(0,pattern.c_str(),path.c_str(),0);

	if( (hFile = _findfirst( pattern.c_str(), &files )) == -1L ){
		morefiles=-1;
	}

	while(morefiles==0){
		SearchHpiFile((char*)(path+files.name).c_str());
		
		morefiles=_findnext( hFile, &files ); 
	}
#endif
}

void CHpiHandler::SearchHpiFile(char* name)
{
	void* hpi=HPIOpen(name);
	if(hpi==0)
		return;
	char file[512];
	int type;
	int size;

//	MessageBox(0,name,"Test",0);
	LRESULT nextFile=HPIGetFiles(hpi, 0, file, &type, &size);

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

	string taDir=".\\";
	FindHpiFilesForDir(taDir+"*.gp4",taDir,dir,found);
	FindHpiFilesForDir(taDir+"*.swx",taDir,dir,found);
	FindHpiFilesForDir(taDir+"*.gp3",taDir,dir,found);
	FindHpiFilesForDir(taDir+"*.ufo",taDir,dir,found);
	FindHpiFilesForDir(taDir+"*.ccx",taDir,dir,found);
	FindHpiFilesForDir(taDir+"*.hpi",taDir,dir,found);

	if (useBackupUnit) 
		FindHpiFilesForDir(taDir+"*.sdu",taDir,dir,found);

	return found;
}

void CHpiHandler::FindHpiFilesForDir(string pattern,string path,string subPath,std::vector<std::string>& found)
{
#ifndef NO_IO
	struct _finddata_t files;    
	long hFile;
	int morefiles=0;

//	MessageBox(0,pattern.c_str(),path.c_str(),0);

	if( (hFile = _findfirst( pattern.c_str(), &files )) == -1L ){
		morefiles=-1;
	}

	while(morefiles==0){
		SearchHpiFileInDir((char*)(path+files.name).c_str(),subPath,found);
		
		morefiles=_findnext( hFile, &files ); 
	}
#endif
}

void CHpiHandler::SearchHpiFileInDir(char* name,string subPath,std::vector<std::string>& found)
{
	void* hpi=HPIOpen(name);
	if(hpi==0)
		return;
	char file[512];
	int type;
	int size;

//	MessageBox(0,name,"Test",0);
	char path[500];
	subPath.erase(subPath.length()-1);
	strcpy(path,subPath.c_str());
	LRESULT nextFile=HPIDir(hpi, 0, path, file, &type, &size);

	while(nextFile!=0){
		if(type==0)
			found.push_back(file);
		nextFile=HPIDir(hpi, nextFile, path, file, &type, &size);
	}
	HPIClose(hpi);
}

