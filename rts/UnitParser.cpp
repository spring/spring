// UnitParser.cpp: implementation of the CUnitParser class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "UnitParser.h"
#include <map>
#include "InfoConsole.h"
#include "FileHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CUnitParser* parser;

CUnitParser::CUnitParser()
{

}

CUnitParser::~CUnitParser()
{
	map<string,UnitInfo*>::iterator ui;
	for(ui=unitInfos.begin();ui!=unitInfos.end();ui++)
		delete ui->second;

	map<string,WeaponInfo*>::iterator wi;
	for(wi=weaponInfos.begin();wi!=weaponInfos.end();wi++)
		delete wi->second;
}

CUnitParser::UnitInfo* CUnitParser::ParseUnit(const string& filename)
{
	map<string,UnitInfo*>::iterator ui;
	if((ui=unitInfos.find(filename))!=unitInfos.end())
			return ui->second;

	UnitInfo* unitinfo=new UnitInfo;
	map<string,string*> stringTokens;
	map<string,float*> floatTokens;
	map<string,int*> intTokens;

	unitinfo->info["type"]="GroundUnit";
	unitinfo->info["health"]="100";
	unitinfo->info["power"]="100";
	unitinfo->info["mass"]="1";

#ifndef NO_WINSTUFF
	string dir="units\\";
#else
	string dir="UNITS/";
#endif
	Parse(dir+filename,unitinfo->info);

	unitInfos[filename]=unitinfo;
	return unitinfo;
}

CUnitParser::WeaponInfo* CUnitParser::ParseWeapon(const string& filename)
{
	map<string,WeaponInfo*>::iterator wi;
	if((wi=weaponInfos.find(filename))!=weaponInfos.end())
			return wi->second;

	WeaponInfo* weaponinfo=new WeaponInfo;

	weaponinfo->info["type"]="Rifle";
	weaponinfo->info["damage"]="10";
	weaponinfo->info["reload"]="1";
	weaponinfo->info["range"]="100";
	weaponinfo->info["projectileSpeed"]="30";
	weaponinfo->info["salvoDelay"]="0.1";
	weaponinfo->info["salvoSize"]="1";
	weaponinfo->info["firesoundvolume"]="1";
	weaponinfo->info["maxAngle"]="180";
#ifndef NO_WINSTUFF
	string dir="weapons\\";
#else
	string dir="Weapons/";
#endif
	Parse(dir+filename,weaponinfo->info);

	weaponInfos[filename]=weaponinfo;
	return weaponinfo;
}

void CUnitParser::MakeLow(std::string &s)
{
	std::string s2=s;
	s="";
	std::string::iterator si;
	for(si=s2.begin();si!=s2.end();si++){
		if(((*si)>='A') && ((*si)<='Z'))
			s+=(char)('a'-'A'+(*si));
		else
			s+=(*si);
	}
}

string CUnitParser::GetWord(CFileHandler& fh)
{
	char a=fh.Peek();
	while(a==' ' || a=='\xd' || a=='\xa'){
		fh.Read(&a,1);
		a=fh.Peek();
		if(fh.Eof())
			break;
	}
	fh.Read(&a,1);
	string s="";
	while(a!=',' && a!=' ' && a!='\xd' && a!='\xa'){
		s+=a;
		fh.Read(&a,1);
		if(fh.Eof())
			break;
	}
	return s;
}

string CUnitParser::GetLine(CFileHandler& fh)
{
	string s="";
	char a;
	fh.Read(&a,1);
	while(a!='\xd' && a!='\xa'){
		s+=a;
		fh.Read(&a,1);
		if(fh.Eof())
			break;
	}
	return s;
}

int CUnitParser::Parse(const string& filename, map<string,string> &info)
{
	CFileHandler ifs(filename);
	string s;
	bool inComment=false;

	while(ifs.Peek()!=EOF){
		s=GetWord(ifs);
		MakeLow(s);
		if(s[0]=='/' && !inComment){
			if(s[1]=='/'){
				continue;
				GetLine(ifs);
			}
			if(s[1]=='*')
				inComment=true;
		}
		if(inComment){
			for(unsigned int a=0;a<s.size()-1;++a)
				if(s[a]=='*' && s[a+1]=='/')
					inComment=false;
			if(inComment)
				continue;
		}
		if(ifs.Eof())
			break;
		while((s.c_str()[0]=='/') || (s.c_str()[0]=='\n')){
			s=GetLine(ifs);
			s=GetWord(ifs);
			MakeLow(s);
			if(ifs.Eof())
				break;
		}
		info[s]=GetWord(ifs);
	}
	return 1;
}
