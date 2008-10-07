#include "StdAfx.h"
#include ".\datastore.h"
#include <fstream>

CDataStore datastore;

using namespace std;
CDataStore::CDataStore(void)
{
	std::ifstream ifs("selectkeys.txt");

	char buf[10000];

	while(ifs.peek()!=EOF && !ifs.eof()){
		ifs >> buf;
		string key(buf);

		if(ifs.peek()==EOF || ifs.eof())
			break;

		ifs >> buf;
		string sel(buf);

		hotkeys[key]=sel;
	}
}

CDataStore::~CDataStore(void)
{
}

void CDataStore::Save(void)
{
	std::ofstream ofs("selectkeys.txt");

	for(map<string,string>::iterator ki=hotkeys.begin();ki!=hotkeys.end();++ki){
		ofs << ki->first;
		ofs << " ";
		ofs << ki->second;
		ofs << "\n";
	}
}
