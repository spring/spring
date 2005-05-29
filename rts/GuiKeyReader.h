#ifndef GUIKEYREADER_H
#define GUIKEYREADER_H
// GuiKeyReader.h: interface for the CGuiKeyReader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIKEYREADER_H__AB32CA41_370E_11D5_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_GUIKEYREADER_H__AB32CA41_370E_11D5_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

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

#endif // !defined(AFX_GUIKEYREADER_H__AB32CA41_370E_11D5_AD55_0080ADA84DE3__INCLUDED_)


#endif /* GUIKEYREADER_H */
