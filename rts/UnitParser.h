#ifndef UNITPARSER_H
#define UNITPARSER_H
// UnitParser.h: interface for the CUnitParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UNITPARSER_H__B436C3C7_876B_44F9_945E_7E66405696F0__INCLUDED_)
#define AFX_UNITPARSER_H__B436C3C7_876B_44F9_945E_7E66405696F0__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include <string>
#include <vector>
#include <map>

class CFileHandler;
using namespace std;

class CUnitParser  
{
public:
	CUnitParser();
	virtual ~CUnitParser();
	
	struct UnitInfo{
		map<string,string> info;
	};
	struct WeaponInfo{
		map<string,string> info;
	};

	map<string,UnitInfo*> unitInfos;
	map<string,WeaponInfo*> weaponInfos;

	UnitInfo* ParseUnit(const string& filename);
	WeaponInfo* ParseWeapon(const string& filename);

private:
	int Parse(const string& filename, map<string,string> &info);
	string GetWord(CFileHandler& fh);
	string GetLine(CFileHandler& fh);
	void MakeLow(std::string &s);

};

extern CUnitParser* parser;

#endif // !defined(AFX_UNITPARSER_H__B436C3C7_876B_44F9_945E_7E66405696F0__INCLUDED_)


#endif /* UNITPARSER_H */
