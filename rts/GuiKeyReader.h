#ifndef GUIKEYREADER_H
#define GUIKEYREADER_H
// GuiKeyReader.h: interface for the CGuiKeyReader class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <map>

using std::string;

class CGuiKeyReader  
{
public:
	int GetKey(string s);
	string TranslateKey(int key);
	CGuiKeyReader(char* filename);
	virtual ~CGuiKeyReader();
	
	std::map<int,string> guiKeys;

protected:
	void CreateKeyNames();
	std::map<std::string,int> keynames;
};

#endif /* GUIKEYREADER_H */
