/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "KeySetSC.h"
#include "KeyCodes.h"
#include "KeyBindings.h"

#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"

#include <SDL_scancode.h>
#include <SDL_keyboard.h>


#define DISALLOW_RELEASE_BINDINGS


/******************************************************************************/
//
// CKeySet
//

void CKeySetSC::Reset()
{
	keySC = -1;
	modifiers = 0;
}

void CKeySetSC::ClearModifiers()
{
	modifiers &= ~(KS_ALT | KS_CTRL | KS_META | KS_SHIFT);
}


void CKeySetSC::SetAnyBit()
{
	ClearModifiers();
	modifiers |= KS_ANYMOD;
}


bool CKeySetSC::IsPureModifier() const
{
	return (CKeyCodes::IsModifier(KeySC()) || (KeySC() == keyBindings.GetFakeMetaKey()));
}


CKeySetSC::CKeySetSC(int k, bool release)
{
	keySC = k;
	modifiers = 0;

	if (KeyInput::GetKeyModState(KMOD_ALT))   { modifiers |= KS_ALT; }
	if (KeyInput::GetKeyModState(KMOD_CTRL))  { modifiers |= KS_CTRL; }
	if (KeyInput::GetKeyModState(KMOD_GUI))   { modifiers |= KS_META; }
	if (KeyInput::GetKeyModState(KMOD_SHIFT)) { modifiers |= KS_SHIFT; }

#ifndef DISALLOW_RELEASE_BINDINGS
	if (release) { modifiers |= KS_RELEASE; }
#endif
}


std::string CKeySetSC::GetString(bool useDefaultKeysym) const
{
	std::string name;
	std::string modstr;

	name = SDL_GetScancodeName(static_cast<SDL_Scancode>(keySC));


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


bool CKeySetSC::ParseModifier(std::string& s, const std::string& token, const std::string& abbr)
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

bool CKeySetSC::Parse(const std::string& token, bool showerror)
{
	Reset();

	std::string s = StringToLower(token);

	// parse the modifiers
	while (!s.empty()) {
		if (ParseModifier(s, "up+",    "u+")) { modifiers |= KS_RELEASE; } else
		if (ParseModifier(s, "any+",   "*+")) { modifiers |= KS_ANYMOD; } else
		if (ParseModifier(s, "alt+",   "a+")) { modifiers |= KS_ALT; } else
		if (ParseModifier(s, "ctrl+",  "c+")) { modifiers |= KS_CTRL; } else
		if (ParseModifier(s, "meta+",  "m+")) { modifiers |= KS_META; } else
		if (ParseModifier(s, "shift+", "s+")) { modifiers |= KS_SHIFT; } else {
			break;
		}
	}

#ifdef DISALLOW_RELEASE_BINDINGS
	modifiers &= ~KS_RELEASE;
#endif

	if (s.empty()) {
		Reset();
		if (showerror) LOG_L(L_ERROR, "KeySetSC: Missing key");
		return false;
	}

	// remove ''s, if present
	if ((s.size() >= 2) && (s[0] == '\'') && (s[s.size() - 1] == '\''))
		s = s.substr(1, s.size() - 2);

	// if key begins with SC_ , i.e: SC_A, we want to know if SDL_SCANCODE_A is a valid scancodename
	if (s.find("sc_") == 0) {

		std::string scanCodeName = s.substr(3);
		int keyScTest = SDL_GetScancodeFromName(scanCodeName.c_str());

		if (keyScTest == SDL_SCANCODE_UNKNOWN) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "Bad Scancodename: %s", scanCodeName.c_str());
			return false;
		}
		else {
			keySC = keyScTest;
		}
	}
	else {
		Reset();
		return false;
	}

	// ToDo: match Scancode Mods instead
	if (keyCodes.IsModifier(keySC))
		modifiers |= KS_ANYMOD;

	if (AnyMod())
		ClearModifiers();

	return true;
}


/******************************************************************************/
//
// CTimedKeyChain
//

void CTimedKeyChainSC::push_back(const int keySC, const spring_time t, const bool isRepeat)
{
	// clear chain on timeout
	const auto dropTime = t - spring_msecs(keyBindings.GetKeyChainTimeout());

	if (!empty() && times.back() < dropTime)
		clear();

	CKeySetSC ks(keySC, false);

	// append repeating keystrokes only wjen they differ from the last
	if (isRepeat && (!empty() && ks == back()))
		return;

	// When you want to press c,alt+b you will press the c(down),c(up),alt(down),b(down)
	// -> the chain will be: c,Alt+alt,Alt+b
	// this should now match "c,alt+b", so we need to get rid of the redundant Alt+alt .
	// Still we want to support "Alt+alt,Alt+alt" (double click alt), so only override e.g. the last in chain
	// when the new keypress is _not_ a modifier.
	if (!empty() && !ks.IsPureModifier() && back().IsPureModifier() && (back().Mod() == ks.Mod())) {
		times.back() = t;
		back() = ks;
		return;
	}

	// push new to front
	CKeyChainSC::emplace_back(ks);
	times.emplace_back(t);
}


