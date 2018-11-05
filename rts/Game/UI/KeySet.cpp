/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "KeySet.h"
#include "KeyCodes.h"
#include "KeyBindings.h"

#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"

#include <SDL_keycode.h>


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


bool CKeySet::IsPureModifier() const
{
	return (CKeyCodes::IsModifier(Key()) || (Key() == keyBindings.GetFakeMetaKey()));
}


CKeySet::CKeySet(int k, bool release)
{
	key = k;
	modifiers = 0;

	if (KeyInput::GetKeyModState(KMOD_ALT))   { modifiers |= KS_ALT; }
	if (KeyInput::GetKeyModState(KMOD_CTRL))  { modifiers |= KS_CTRL; }
	if (KeyInput::GetKeyModState(KMOD_GUI))   { modifiers |= KS_META; }
	if (KeyInput::GetKeyModState(KMOD_SHIFT)) { modifiers |= KS_SHIFT; }

#ifndef DISALLOW_RELEASE_BINDINGS
	if (release) { modifiers |= KS_RELEASE; }
#endif
}


std::string CKeySet::GetString(bool useDefaultKeysym) const
{
	std::string name;
	std::string modstr;

	if (useDefaultKeysym) {
		name = keyCodes.GetDefaultName(key);
	} else {
		name = keyCodes.GetName(key);
	}
	
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


bool CKeySet::Parse(const std::string& token, bool showerror)
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
		if (showerror) LOG_L(L_ERROR, "KeySet: Missing key");
		return false;
	}

	// remove ''s, if present
	if ((s.size() >= 2) && (s[0] == '\'') && (s[s.size() - 1] == '\''))
		s = s.substr(1, s.size() - 2);

	if (s.find("0x") == 0) {
		const char* start = (s.c_str() + 2);
		char* end;
		key = strtol(start, &end, 16);
		if (end == start) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "KeySet: Bad hex value: %s", s.c_str());
			return false;
		}
	} else {
		if ((key = keyCodes.GetCode(s)) < 0) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "KeySet: Bad keysym: %s", s.c_str());
			return false;
		}
	}

	if (keyCodes.IsModifier(key))
		modifiers |= KS_ANYMOD;

	if (AnyMod())
		ClearModifiers();
	
	return true;
}


/******************************************************************************/
//
// CTimedKeyChain
//

void CTimedKeyChain::push_back(const int key, const spring_time t, const bool isRepeat)
{
	// clear chain on timeout
	const auto dropTime = t - spring_msecs(keyBindings.GetKeyChainTimeout());

	if (!empty() && times.back() < dropTime)
		clear();

	CKeySet ks(key, false);

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
	CKeyChain::emplace_back(ks);
	times.emplace_back(t);
}
