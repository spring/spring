#ifndef HPIHANDLER_H
#define HPIHANDLER_H
// HpiHandler.h: interface for the CHpiHandler class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#if !defined(AFX_HPIHANDLER_H__68DF4969_8792_4893_98F7_8092C36479D7__INCLUDED_)
#define AFX_HPIHANDLER_H__68DF4969_8792_4893_98F7_8092C36479D7__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include "hpiutil2/hpiutil.h"

using std::string;
using std::map;

class CHpiHandler  
{
public:
	void MakeLower(char* s);
	void MakeLower(string &s);
	int LoadFile(string name,void* buffer);
	int GetFileSize(string name);
	CHpiHandler();
	virtual ~CHpiHandler();

	struct FileData{
		string hpiname;
		int size;
	};
	map<string,FileData> files;
	map<string,hpifile*> datafiles;
	hpifile *locatehpi(const char *name);
	std::vector<std::string> GetFilesInDir(std::string dir);
private:
	void FindHpiFiles(string pattern,string path);
	void RegisterFile(char* hpi,char* name,int size);
	void SearchHpiFile(char* name);

	void FindHpiFilesForDir(string pattern,string path,string subPath,std::vector<std::string>& found);
	void SearchHpiFileInDir(char* name,string subPath,std::vector<std::string>& found);

	HINSTANCE m_hDLL;
	bool useBackupUnit;
};

extern CHpiHandler* hpiHandler;

#endif // !defined(AFX_HPIHANDLER_H__68DF4969_8792_4893_98F7_8092C36479D7__INCLUDED_)


#endif /* HPIHANDLER_H */
