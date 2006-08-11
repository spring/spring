#include "StdAfx.h"
// CKeyBindings.cpp: implementation of the CKeyBindings class.
//
//////////////////////////////////////////////////////////////////////

#include "KeySet.h"
#include "KeyCodes.h"
#include "SDL_keysym.h"
#include "SDL_types.h"
#include <stdlib.h>

extern Uint8* keys; // from System/Main.cpp


/******************************************************************************/
//
// CKeySet
//

void CKeySet::Reset()
{
	key = -1;
	modifiers = 0;
}


void CKeySet::ClearModifiers()
{
	modifiers &= ~(KS_ALT | KS_CTRL | KS_META | KS_SHIFT);
}


CKeySet::CKeySet(int k, bool release)
{
	key = k;
	modifiers = 0;
	if (keys[SDLK_LALT])   { modifiers |= KS_ALT; }  		
	if (keys[SDLK_LCTRL])  { modifiers |= KS_CTRL; }		
	if (keys[SDLK_LMETA])  { modifiers |= KS_META; }		
	if (keys[SDLK_LSHIFT]) { modifiers |= KS_SHIFT; }
	if (release)           { modifiers |= KS_RELEASE; }
}


string CKeySet::GetString() const
{
	string name = keyCodes->GetName(key);
	if (name.empty()) {
		return "";
	}
	
	string modstr;
	if (modifiers & KS_RELEASE) { modstr += "Up+"; }
	if (modifiers & KS_ANYMOD)  { modstr += "Any+"; }
	if (modifiers & KS_ALT)     { modstr += "Alt+"; }
	if (modifiers & KS_CTRL)    { modstr += "Ctrl+"; }
	if (modifiers & KS_META)    { modstr += "Meta+"; }
	if (modifiers & KS_SHIFT)   { modstr += "Shift+"; }
	
	return (modstr + name);
}


bool CKeySet::ParseModifier(string& s, const string& token, const string& abbr)
{
	if (s.find(token) == 0) {
		s.erase(0, token.size());
		return true;
	}
	if (s.find(abbr) == 0) {

		s.erase(0, abbr.size());
		return true;
	}
	return false;
}


bool CKeySet::Parse(const string& token)
{
	Reset();

	string s = StringToLower(token);

	// parse the modifiers
	while (!s.empty()) {
		     if (ParseModifier(s, "up+",    "u+")) { modifiers |= KS_RELEASE; }
		else if (ParseModifier(s, "any+",   "*+")) { modifiers |= KS_ANYMOD; }
		else if (ParseModifier(s, "alt+",   "a+")) { modifiers |= KS_ALT; }
		else if (ParseModifier(s, "ctrl+",  "c+")) { modifiers |= KS_CTRL; }
		else if (ParseModifier(s, "meta+",  "m+")) { modifiers |= KS_META; }
		else if (ParseModifier(s, "shift+", "s+")) { modifiers |= KS_SHIFT; }
		else {
			break;
		}
	}
	
	// remove ''s, if present
	if ((s.size() >= 2) && (s[0] == '\'') && (s[s.size() - 1] == '\'')) {
		s = s.substr(1, s.size() - 2);
	}

	if (s.find("0x") == 0) {
		const char* start = (s.c_str() + 2);
		char* end;
		key = strtol(start, &end, 16);
		if (end == start) {
			Reset();
			printf("Bad hex value: %s\n", s.c_str());
			return false;
		}
	}
	else {
		key = keyCodes->GetCode(s);
		if (key < 0) {
			Reset();
			printf("Bad keysym: %s\n", s.c_str());
			return false;
		}
	}
	
	if (keyCodes->IsModifier(key)) {
		modifiers |= KS_ANYMOD;
	}
	if (AnyMod()) {
		ClearModifiers();
	}
	
	return true;
}
