/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cstdlib>
#include <boost/cstdint.hpp>
#include <SDL_keysym.h>

#include "StdAfx.h"
#include "mmgr.h"

#include "KeySet.h"
#include "KeyCodes.h"

#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/Input/KeyInput.h"

#define DISALLOW_RELEASE_BINDINGS


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


void CKeySet::SetAnyBit()
{
	ClearModifiers();
	modifiers |= KS_ANYMOD;
}


CKeySet::CKeySet(int k, bool release)
{
	key = k;
	modifiers = 0;

	if (keyInput->IsKeyPressed(SDLK_LALT))   { modifiers |= KS_ALT; }
	if (keyInput->IsKeyPressed(SDLK_LCTRL))  { modifiers |= KS_CTRL; }
	if (keyInput->IsKeyPressed(SDLK_LMETA))  { modifiers |= KS_META; }
	if (keyInput->IsKeyPressed(SDLK_LSHIFT)) { modifiers |= KS_SHIFT; }

#ifndef DISALLOW_RELEASE_BINDINGS
	if (release) { modifiers |= KS_RELEASE; }
#endif
}


std::string CKeySet::GetString(bool useDefaultKeysym) const
{
	std::string name;
	if (useDefaultKeysym) {
		name = keyCodes->GetDefaultName(key);
	} else {
		name = keyCodes->GetName(key);
	}
	
	std::string modstr;
#ifndef DISALLOW_RELEASE_BINDINGS
	if (modifiers & KS_RELEASE) { modstr += "Up+"; }
#endif
	if (modifiers & KS_ANYMOD)  { modstr += "Any+"; }
	if (modifiers & KS_ALT)     { modstr += "Alt+"; }
	if (modifiers & KS_CTRL)    { modstr += "Ctrl+"; }
	if (modifiers & KS_META)    { modstr += "Meta+"; }
	if (modifiers & KS_SHIFT)   { modstr += "Shift+"; }
	
	return (modstr + name);
}


bool CKeySet::ParseModifier(std::string& s, const std::string& token, const std::string& abbr)
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


bool CKeySet::Parse(const std::string& token)
{
	Reset();

	std::string s = StringToLower(token);

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

#ifdef DISALLOW_RELEASE_BINDINGS
	modifiers &= ~KS_RELEASE;
#endif
	
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
			logOutput.Print("Bad hex value: %s\n", s.c_str());
			return false;
		}
		if (key >= SDLK_LAST) {
			Reset();
			logOutput.Print("Hex value out of range: %s\n", s.c_str());
			return false;
		}
	}
	else {
		key = keyCodes->GetCode(s);
		if (key < 0) {
			Reset();
			logOutput.Print("Bad keysym: %s\n", s.c_str());
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
