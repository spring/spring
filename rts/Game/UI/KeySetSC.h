/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYSETSC_H
#define KEYSETSC_H

#include <string>
#include <deque>
#include "System/Misc/SpringTime.h"


class CKeySetSC {
	public:
		CKeySetSC() { Reset(); }
		CKeySetSC(int keySC, bool release);

		void Reset();
		void SetAnyBit();
		void ClearModifiers();
		bool Parse(const std::string& token, bool showerror = true);

		std::string GetString(bool useDefaultKeysym) const;

		enum CKeySetModifiers {
			KS_ALT     = (1 << 0),
			KS_CTRL    = (1 << 1),
			KS_META    = (1 << 2),
			KS_SHIFT   = (1 << 3),
			KS_ANYMOD  = (1 << 4),
			KS_RELEASE = (1 << 5)
		};

		int  KeySC()     const { return keySC; }
		unsigned char Mod() const { return modifiers; }
		bool Alt()     const { return !!(modifiers & KS_ALT); }
		bool Ctrl()    const { return !!(modifiers & KS_CTRL); }
		bool Meta()    const { return !!(modifiers & KS_META); }
		bool Shift()   const { return !!(modifiers & KS_SHIFT); }
		bool AnyMod()  const { return !!(modifiers & KS_ANYMOD); }
		bool Release() const { return !!(modifiers & KS_RELEASE); }

		bool IsPureModifier() const;

		bool operator<(const CKeySetSC& ks) const
		{
			if (keySC < ks.keySC) { return true; }
			if (keySC > ks.keySC) { return false; }
			if (modifiers < ks.modifiers) { return true; }
			if (modifiers > ks.modifiers) { return false; }
			return false;
		}

		bool fit(const CKeySetSC& ks) const
		{
			return (keySC == ks.keySC) && ((modifiers == ks.modifiers) || AnyMod() || ks.AnyMod());
		}

		bool operator==(const CKeySetSC& ks) const
		{
			return ((keySC == ks.keySC) && (modifiers == ks.modifiers));
		}

		bool operator!=(const CKeySetSC& ks) const
		{
			return ((keySC != ks.keySC) || (modifiers != ks.modifiers));
		}

	protected:
		bool ParseModifier(std::string& s, const std::string& token, const std::string& abbr);

	protected:
		int keySC;
		unsigned char modifiers;
};


class CKeyChainSC : public std::deque<CKeySetSC>
{
	public:
		bool operator<(const CKeyChainSC& kc) const
		{
			if (size() < kc.size()) { return true;  }
			if (size() > kc.size()) { return false; }
			if (empty())            { return false; }
			return (back() < kc.back());
		}

		bool operator==(const CKeyChainSC& needle) const
		{
			if (size() != needle.size())
				return false;

			return std::equal(needle.rbegin(), needle.rend(), rbegin());
		}

		bool fit(const CKeyChainSC& needle) const
		{
			// Scans the chain (*this) for a needle from the _back_
			// e.g. chain=a,b,c,d will fit needle=b,c,d but won't fit needle=a,b,c!

			if (size() < needle.size())
				return false;

			return std::equal(needle.rbegin(), needle.rend(), rbegin(), [](const CKeySetSC& a, const CKeySetSC& b) { return b.fit(a); });
		}

		std::string GetString() const
		{
			std::string s;
			for (const CKeySetSC& ks : *this) {
				if (!s.empty()) s += ",";
				s += ks.GetString(true);
			}
			return s;
		}
};


class CTimedKeyChainSC : public CKeyChainSC
{
	public:
		std::deque<spring_time> times;

		void clear()
		{
			CKeyChainSC::clear();
			times.clear();
		}

		void push_back(const int keySC, const spring_time t, const bool isRepeat);
		void emplace_back(const CKeySetSC& ks, const spring_time t) { assert(false); }
};


#endif /* KEYSET_H */
