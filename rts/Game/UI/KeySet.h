/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYSET_H
#define KEYSET_H

#include <string>
#include <map>

class CKeySet {
	public:
		CKeySet() { Reset(); }
		CKeySet(int key, bool release);

		void Reset();
		void SetAnyBit();
		void ClearModifiers();
		bool Parse(const std::string& token);

		std::string GetString(bool useDefaultKeysym) const;

		enum CKeySetModifiers {
			KS_ALT     = (1 << 0),
			KS_CTRL    = (1 << 1),
			KS_META    = (1 << 2),
			KS_SHIFT   = (1 << 3),
			KS_ANYMOD  = (1 << 4),
			KS_RELEASE = (1 << 5)
		};

		int  Key()     const { return key; }
		bool Alt()     const { return !!(modifiers & KS_ALT); }
		bool Ctrl()    const { return !!(modifiers & KS_CTRL); }
		bool Meta()    const { return !!(modifiers & KS_META); }
		bool Shift()   const { return !!(modifiers & KS_SHIFT); }
		bool AnyMod()  const { return !!(modifiers & KS_ANYMOD); }
		bool Release() const { return !!(modifiers & KS_RELEASE); }

		bool operator<(const CKeySet& ks) const
		{
			if (key < ks.key) { return true; }
			if (key > ks.key) { return false; }
			if (modifiers < ks.modifiers) { return true; }
			if (modifiers > ks.modifiers) { return false; }
			return false;
		}

		bool operator==(const CKeySet& ks) const
		{
			return ((key == ks.key) && (modifiers == ks.modifiers));
		}

		bool operator!=(const CKeySet& ks) const
		{
			return ((key != ks.key) || (modifiers != ks.modifiers));
		}

	protected:
		bool ParseModifier(std::string& s, const std::string& token, const std::string& abbr);

	protected:
		int key;
		unsigned char modifiers;
};


#endif /* KEYSET_H */
