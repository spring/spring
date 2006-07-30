#include "StdAfx.h"
// GuiKeyReader.cpp: implementation of the CGuiKeyReader class.
//
//////////////////////////////////////////////////////////////////////

#include "GuiKeyReader.h"
#include <algorithm>
#include <cctype>
#include "FileSystem/FileHandler.h"
#include "Platform/errorhandler.h"
#include "SDL_keysym.h"

#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static void GetWord(CFileHandler& fh, std::string &s)
{
	char a = fh.Peek();
	while (a == ' ' || a == '\n' || a == '\r') {
		fh.Read(&a, 1);
		a = fh.Peek();
		if (fh.Eof())
			break;
	}
	s = "";
	fh.Read(&a, 1);
	while (a != ',' && a != ' ' && a != '\n' && a != '\r') {
		s += a;
		fh.Read(&a, 1);
		if (fh.Eof())
			break;
	}
}

static std::string GetLine(CFileHandler& fh)
{
	std::string s = "";
	char a;
	fh.Read(&a, 1);
	while (a!='\n' && a!='\r') {
		s += a;
		fh.Read(&a, 1);
		if (fh.Eof())
			break;
	}
	return s;
}

static void MakeLow(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::tolower);
}

CGuiKeyReader::CGuiKeyReader(char* filename)
{
	CreateKeyNames();	
	CFileHandler ifs(filename);
	std::string s;
	bool inComment = false;

	while (ifs.Peek() != EOF) {
		GetWord(ifs, s);
		if (s[0] == '/' && !inComment) {
			if (s.size() > 1 && s[1] == '*')
				inComment=true;
		}
		if (inComment) {
			for (unsigned int a = 0; a < s.size() - 1; ++a)
				if (s[a] == '*' && s[a+1] == '/')
					inComment = false;
			if (inComment)
				continue;
		}
		MakeLow(s);
		if (ifs.Eof())
			break;
		while ((s.c_str()[0] == '/') || (s.c_str()[0] == '\n')) {
			GetLine(ifs);
			GetWord(ifs, s);
			MakeLow(s);
			if(ifs.Eof())
				break;
		}
		if (ifs.Peek()==EOF)
			break;
		if (s.compare("bind") == 0){
			GetWord(ifs, s);
			MakeLow(s);
			int i = GetKey(s);
			if (i < 0) {
				handleerror(0, s.c_str(), "Unknown key", 0);
			}
			GetWord(ifs, s);
			MakeLow(s);
			guiKeys[i] = s;
		} else { 
			handleerror(0,"Couldnt parse control file",s.c_str(),0);
		}
		GetLine(ifs);
	}
}

  

CGuiKeyReader::~CGuiKeyReader()
{
}


bool CGuiKeyReader::Bind(std::string key, std::string action)
{
  int k = GetKey(key);  
  if (k < 0) {
  	return false;
	}
	MakeLow(action);
	guiKeys[k] = action;
	return true;
}


std::string CGuiKeyReader::TranslateKey(int key)
{
	return guiKeys[key];
}


void CGuiKeyReader::CreateKeyNames()
{
	/* The keyboard syms have been cleverly chosen to map to ASCII */
	keynames["backspace"] = SDLK_BACKSPACE;
	keynames["tab"] = SDLK_TAB;
	keynames["clear"] = SDLK_CLEAR;
	keynames["return"] = SDLK_RETURN;
	keynames["enter"] = SDLK_RETURN;
	keynames["pause"] = SDLK_PAUSE;
	keynames["esc"] = SDLK_ESCAPE;
	keynames["escape"] = SDLK_ESCAPE;
	keynames["space"] = SDLK_SPACE;
	/* Everything from SDLK_EXCLAIM to SDLK_z is handler in GetKey() */
	keynames["delete"] = SDLK_DELETE;
	/* End of ASCII mapped keysyms */

	keynames["\xA7"] = 220;

	/* Numeric keypad */
	keynames["numpad0"] = SDLK_KP0;
	keynames["numpad1"] = SDLK_KP1;
	keynames["numpad2"] = SDLK_KP2;
	keynames["numpad3"] = SDLK_KP3;
	keynames["numpad4"] = SDLK_KP4;
	keynames["numpad5"] = SDLK_KP5;
	keynames["numpad6"] = SDLK_KP6;
	keynames["numpad7"] = SDLK_KP7;
	keynames["numpad8"] = SDLK_KP8;
	keynames["numpad9"] = SDLK_KP9;
	keynames["numpad."] = SDLK_KP_PERIOD;
	keynames["numpad/"] = SDLK_KP_DIVIDE;
	keynames["numpad*"] = SDLK_KP_MULTIPLY;
	keynames["numpad-"] = SDLK_KP_MINUS;
	keynames["numpad+"] = SDLK_KP_PLUS;
	keynames["numpad_enter"] = SDLK_KP_ENTER;
	keynames["numpad="] = SDLK_KP_EQUALS;

	/* Arrows + Home/End pad */
	keynames["up"] = SDLK_UP;
	keynames["down"] = SDLK_DOWN;
	keynames["right"] = SDLK_RIGHT;
	keynames["left"] = SDLK_LEFT;
	keynames["insert"] = SDLK_INSERT;
	keynames["home"] = SDLK_HOME;
	keynames["end"] = SDLK_END;
	keynames["pageup"] = SDLK_PAGEUP;
	keynames["pagedown"] = SDLK_PAGEDOWN;

	/* Function keys */
	keynames["f1"] = SDLK_F1;
	keynames["f2"] = SDLK_F2;
	keynames["f3"] = SDLK_F3;
	keynames["f4"] = SDLK_F4;
	keynames["f5"] = SDLK_F5;
	keynames["f6"] = SDLK_F6;
	keynames["f7"] = SDLK_F7;
	keynames["f8"] = SDLK_F8;
	keynames["f9"] = SDLK_F9;
	keynames["f10"] = SDLK_F10;
	keynames["f11"] = SDLK_F11;
	keynames["f12"] = SDLK_F12;
	keynames["f13"] = SDLK_F13;
	keynames["f14"] = SDLK_F14;
	keynames["f15"] = SDLK_F15;

	/* Key state modifier keys */
	//keynames["numlock"] = SDLK_NUMLOCK;
	//keynames["capslock"] = SDLK_CAPSLOCK;
	//keynames["scrollock"] = SDLK_SCROLLOCK;
	keynames["shift"] = SDLK_LSHIFT;
	keynames["ctrl"] = SDLK_LCTRL;
	keynames["alt"] = SDLK_LALT;
	keynames["meta"] = SDLK_LMETA;
	// I doubt these can be used correctly anyway (without special support in other parts of the spring code...)
	//keynames["super"] = SDLK_LSUPER;    /* Left "Windows" key */
	//keynames["mode"] = SDLK_MODE;       /* "Alt Gr" key */
	//keynames["compose"] = SDLK_COMPOSE; /* Multi-key compose key */

	/* Miscellaneous function keys */
	//keynames["help"] = SDLK_HELP;
	keynames["printscreen"] = SDLK_PRINT;
	//keynames["sysreq"] = SDLK_SYSREQ;
	//keynames["break"] = SDLK_BREAK;
	//keynames["menu"] = SDLK_MENU;
	// These conflict with "joy*" keynames.
	//keynames["power"] = SDLK_POWER;     /* Power Macintosh power key */
	//keynames["euro"] = SDLK_EURO;       /* Some european keyboards */
	//keynames["undo"] = SDLK_UNDO;       /* Atari keyboard has Undo */

	// Are these required for someone??
	//keynames["'<'"] = 226;
	//keynames["','"] = 188;
	//keynames["'.'"] = 190;

	keynames["joyx"] = 400;
	keynames["joyy"] = 401;
	keynames["joyz"] = 402;
	keynames["joyw"] = 403;
	keynames["joy0"] = 300;
	keynames["joy1"] = 301;
	keynames["joy2"] = 302;
	keynames["joy3"] = 303;
	keynames["joy4"] = 304;
	keynames["joy5"] = 305;
	keynames["joy6"] = 306;
	keynames["joy7"] = 307;
	keynames["joyup"] = 320;
	keynames["joydown"] = 321;
	keynames["joyleft"] = 322;
	keynames["joyright"] = 323;
}

int CGuiKeyReader::GetKey(std::string s)
{
	if ((s[0] >= '0') && (s[0] <= '9'))
		return strtol(s.c_str(), NULL, 0);

	if (s[0] == '\'' && s[s.length() - 1] == '\'')
		s = s.substr(1, s.length() - 2);

	if (s.length() == 1 && s[0] >= SDLK_EXCLAIM && s[0] <= SDLK_z)
		return std::tolower(s[0]);

	int a = keynames[s];
	if(a == 0)
		return -1;
	return a;
}
