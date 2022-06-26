/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "KeySet.h"
#include "KeyCodes.h"
#include "ScanCodes.h"
#include "KeyBindings.h"

#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Input/KeyInput.h"

#include <SDL_keycode.h>


/******************************************************************************/
//
// CKeySet
//

void CKeySet::Reset()
{
	key = -1;
	modifiers = 0;
	type = KSKeyCode;
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
	return (keyCodes.IsModifier(Key()) || (Key() == keyBindings.GetFakeMetaKey()));
}

bool CKeySet::IsModifier() const
{
	return GetKeys().IsModifier(key);
}

bool CKeySet::IsKeyCode() const
{
	return type == KSKeyCode;
}

IKeys CKeySet::GetKeys() const
{
	return IsKeyCode() ? (IKeys)keyCodes : (IKeys)scanCodes;
}

CKeySet::CKeySet(int k)
	: CKeySet(k, KSKeyCode) { }
CKeySet::CKeySet(int k, CKeySetType keyType)
	: CKeySet(k, GetCurrentModifiers(), keyType) { }
CKeySet::CKeySet(int k, unsigned char mods, CKeySetType keyType)
{
	type = keyType;
	key = k;
	modifiers = mods;
}


unsigned char CKeySet::GetCurrentModifiers()
{
	unsigned char modifiers = 0;

	if (KeyInput::GetKeyModState(KMOD_ALT))   { modifiers |= KS_ALT; }
	if (KeyInput::GetKeyModState(KMOD_CTRL))  { modifiers |= KS_CTRL; }
	if (KeyInput::GetKeyModState(KMOD_GUI))   { modifiers |= KS_META; }
	if (KeyInput::GetKeyModState(KMOD_SHIFT)) { modifiers |= KS_SHIFT; }

	return modifiers;
}


std::string CKeySet::GetHumanModifiers(unsigned char modifiers)
{
	std::string modstr;

	if (modifiers & KS_ANYMOD)  { modstr += "Any+"; }
	if (modifiers & KS_ALT)     { modstr += "Alt+"; }
	if (modifiers & KS_CTRL)    { modstr += "Ctrl+"; }
	if (modifiers & KS_META)    { modstr += "Meta+"; }
	if (modifiers & KS_SHIFT)   { modstr += "Shift+"; }

	return modstr;
}


std::string CKeySet::GetString(bool useDefaultKeysym) const
{
	std::string name;

	IKeys keys = GetKeys();
	name = useDefaultKeysym ? keys.GetDefaultName(key) : keys.GetName(key);
	
	return (GetHumanModifiers(modifiers) + name);
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
		if (ParseModifier(s, "up+",    "u+")) { LOG_L(L_WARNING, "KeySet: Up modifier is deprecated"); } else
		if (ParseModifier(s, "any+",   "*+")) { modifiers |= KS_ANYMOD; } else
		if (ParseModifier(s, "alt+",   "a+")) { modifiers |= KS_ALT; } else
		if (ParseModifier(s, "ctrl+",  "c+")) { modifiers |= KS_CTRL; } else
		if (ParseModifier(s, "meta+",  "m+")) { modifiers |= KS_META; } else
		if (ParseModifier(s, "shift+", "s+")) { modifiers |= KS_SHIFT; } else {
			break;
		}
	}

	if (s.empty()) {
		Reset();
		if (showerror) LOG_L(L_ERROR, "KeySet: Missing key");
		return false;
	}

	// remove ''s, if present
	if ((s.size() >= 2) && (s[0] == '\'') && (s[s.size() - 1] == '\''))
		s = s.substr(1, s.size() - 2);

	if (s.find("0x") == 0) {
		type = KSKeyCode;

		const char* start = (s.c_str() + 2);
		char* end;
		key = strtol(start, &end, 16);
		if (end == start) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "KeySet: Bad hex value: %s", s.c_str());
			return false;
		}
	} else if (s.find("sc_0x") == 0) {
		type = KSScanCode;

		const char* start = (s.c_str() + 5);
		char* end;
		key = strtol(start, &end, 16);
		if (end == start) {
			Reset();
			if (showerror) LOG_L(L_ERROR, "KeySet: Bad hex value: %s", s.c_str());
			return false;
		}
	} else if (((key = keyCodes.GetCode(s)) > 0)) {
		type = KSKeyCode;
	} else if (((key = scanCodes.GetCode(s)) > 0)) {
		type = KSScanCode;
	} else {
		Reset();
		if (showerror) LOG_L(L_ERROR, "KeySet: Bad keysym: %s", s.c_str());
		return false;
	}

	if (IsModifier())
		modifiers |= KS_ANYMOD;

	if (AnyMod())
		ClearModifiers();
	
	return true;
}


/******************************************************************************/
//
// CTimedKeyChain
//

void CTimedKeyChain::push_back(const CKeySet& ks, const spring_time t, const bool isRepeat)
{
	// clear chain on timeout
	const auto dropTime = t - spring_msecs(keyBindings.GetKeyChainTimeout());

	if (!empty() && times.back() < dropTime)
		clear();

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
