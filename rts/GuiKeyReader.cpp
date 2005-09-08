#include "StdAfx.h"
// GuiKeyReader.cpp: implementation of the CGuiKeyReader class.
//
//////////////////////////////////////////////////////////////////////

#include "GuiKeyReader.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <cctype>
#include "FileHandler.h"
#include "errorhandler.h"
#include "SDL_keysym.h"

//#include "mmgr.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static void GetWord(CFileHandler& fh,std::string &s)
{
	char a=fh.Peek();
	while(a==' ' || a=='\xd' || a=='\xa'){
		fh.Read(&a,1);
		a=fh.Peek();
		if(fh.Eof())
			break;
	}
	s="";
	fh.Read(&a,1);
	while(a!=',' && a!=' ' && a!='\xd' && a!='\xa'){
		s+=a;
		fh.Read(&a,1);
		if(fh.Eof())
			break;
	}
}

static string GetLine(CFileHandler& fh)
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

static void MakeLow(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::tolower);
/*
	std::string s2=s;
	s="";
	std::string::iterator si;
	for(si=s2.begin();si!=s2.end();si++){
		if(((*si)>='A') && ((*si)<='Z'))
			s+=(char)('a'-'A'+(*si));
		else
			s+=(*si);
	}*/
}

CGuiKeyReader::CGuiKeyReader(char* filename)
{
	CreateKeyNames();	
	CFileHandler ifs(filename);
	string s;
	bool inComment=false;;

	while(ifs.Peek()!=EOF){
		GetWord(ifs,s);
		if(s[0]=='/' && !inComment){
			if(s.size()>1 && s[1]=='*')
				inComment=true;
		}
		if(inComment){
			for(unsigned int a=0;a<s.size()-1;++a)
				if(s[a]=='*' && s[a+1]=='/')
					inComment=false;
			if(inComment)
				continue;
		}
		MakeLow(s);
		if(ifs.Eof())
			break;
		while((s.c_str()[0]=='/') || (s.c_str()[0]=='\n')){
			GetLine(ifs);
			GetWord(ifs,s);
			MakeLow(s);
			if(ifs.Eof())
				break;
		}
		if(ifs.Peek()==EOF)
			break;
		if(s.compare("bind")==0){
			GetWord(ifs,s);
			MakeLow(s);
			int i=GetKey(s);
			GetWord(ifs,s);
			MakeLow(s);
			guiKeys[i]=s;			
		} else 
			handleerror(0,"Couldnt parse control file",s.c_str(),0);
		GetLine(ifs);
	}
}

CGuiKeyReader::~CGuiKeyReader()
{

}

string CGuiKeyReader::TranslateKey(int key)
{
	return guiKeys[key];
}


void CGuiKeyReader::CreateKeyNames()
{
	std::string s="\'";
	for(char c='a';c<='z';c++)
		keynames[s+c+s]=SDLK_FIRST+c;

	keynames["\'0\'"]=SDLK_0;
	keynames["\'1\'"]=SDLK_1;
	keynames["\'2\'"]=SDLK_2;
	keynames["\'3\'"]=SDLK_3;
	keynames["\'4\'"]=SDLK_4;
	keynames["\'5\'"]=SDLK_5;
	keynames["\'6\'"]=SDLK_6;
	keynames["\'7\'"]=SDLK_7;
	keynames["\'8\'"]=SDLK_8;
	keynames["\'9\'"]=SDLK_9;
	keynames["space"]=SDLK_SPACE;
	keynames["enter"]=SDLK_RETURN;
	keynames["backspace"]=SDLK_BACKSPACE;
	keynames["esc"]=SDLK_ESCAPE;
	keynames["f1"]=SDLK_F1;
	keynames["f2"]=SDLK_F2;
	keynames["f3"]=SDLK_F3;
	keynames["f4"]=SDLK_F4;
	keynames["f5"]=SDLK_F5;
	keynames["f6"]=SDLK_F6;
	keynames["f7"]=SDLK_F7;
	keynames["f8"]=SDLK_F8;
	keynames["f9"]=SDLK_F9;
	keynames["f10"]=SDLK_F10;
	keynames["f11"]=SDLK_F11;
	keynames["f12"]=SDLK_F12;
	keynames["printscreen"]=SDLK_PRINT;
	keynames["§"]=220;
	keynames["`"]=SDLK_BACKQUOTE;
	keynames["+"]=SDLK_PLUS;
	keynames["-"]=SDLK_MINUS;
	keynames["shift"]=SDLK_LSHIFT;
	keynames["alt"]=SDLK_LALT;
	keynames["tilde"]=SDLK_BACKQUOTE;
	keynames[s+'<'+s]=226;
	keynames[s+','+s]=188;
	keynames[s+'.'+s]=190;
	keynames["insert"]=SDLK_INSERT;
	keynames["delete"]=SDLK_DELETE;
	keynames["home"]=SDLK_HOME;
	keynames["end"]=SDLK_END;
	keynames["pageup"]=SDLK_PAGEUP;
	keynames["pagedown"]=SDLK_PAGEDOWN;
	keynames["numpad0"]=SDLK_KP0;
	keynames["numpad1"]=SDLK_KP1;
	keynames["numpad2"]=SDLK_KP2;
	keynames["numpad3"]=SDLK_KP3;
	keynames["numpad4"]=SDLK_KP4;
	keynames["numpad5"]=SDLK_KP5;
	keynames["numpad6"]=SDLK_KP6;
	keynames["numpad7"]=SDLK_KP7;
	keynames["numpad8"]=SDLK_KP8;
	keynames["numpad9"]=SDLK_KP9;
	keynames["numpad+"]=SDLK_KP_PLUS;
	keynames["numpad-"]=SDLK_KP_MINUS;
	keynames["numpad*"]=SDLK_KP_MULTIPLY;
	keynames["numpad/"]=SDLK_KP_DIVIDE;
	keynames["ctrl"]=SDLK_LCTRL;
	keynames["up"]=SDLK_UP;
	keynames["down"]=SDLK_DOWN;
	keynames["left"]=SDLK_LEFT;
	keynames["right"]=SDLK_RIGHT;
	keynames["pause"]=	SDLK_BREAK;
	keynames["joyx"]=400;
	keynames["joyy"]=401;
	keynames["joyz"]=402;
	keynames["joyw"]=403;
	keynames["joy0"]=300;
	keynames["joy1"]=301;
	keynames["joy2"]=302;
	keynames["joy3"]=303;
	keynames["joy4"]=304;
	keynames["joy5"]=305;
	keynames["joy6"]=306;
	keynames["joy7"]=307;
	keynames["joyup"]=320;
	keynames["joydown"]=321;
	keynames["joyleft"]=322;
	keynames["joyright"]=323;
}

int CGuiKeyReader::GetKey(string s)
{
	if((s[0]>'0') && (s[0]<='9'))
		return atoi(s.c_str());
	
	int a=keynames[s];
	if(a==0)
		handleerror(0,s.c_str(),"Unknown key",0);
	return a;

}
