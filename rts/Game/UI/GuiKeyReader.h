#ifndef GUIKEYREADER_H
#define GUIKEYREADER_H
// GuiKeyReader.h: interface for the CGuiKeyReader class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <map>


class CGuiKeyReader
{
public:
	std::string TranslateKey(int key);
	CGuiKeyReader(char* filename);
	virtual ~CGuiKeyReader();
	bool Bind(std::string key, std::string action);

	std::map<int, std::string> guiKeys;
protected:
	int GetKey(std::string s);
	void CreateKeyNames();
	std::map<std::string, int> keynames;
};


#endif /* GUIKEYREADER_H */
