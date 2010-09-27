/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEY_AUTO_BINDER_H
#define KEY_AUTO_BINDER_H

#include <string>
#include <vector>
#include <set>

#include "Lua/LuaHandle.h"


struct UnitDef;
struct lua_State;

class CKeyAutoBinder : public CLuaHandle
{
	public:
		CKeyAutoBinder();
		~CKeyAutoBinder();

		bool BindBuildType(const string& keystr,
		                   const vector<string>& requirements,
		                   const vector<string>& sortCriteria,
		                   const vector<string>& chords);

	private:
		string LoadFile(const string& filename) const;
		bool LoadCode(const string& code, const string& debug);
		bool LoadCompareFunc();
		string MakeRequirementCall(const vector<string>& requirements);
		string MakeSortCriteriaCall(const vector<string>& sortCriteria);
		string AddUnitDefPrefix(const string& text, const string& pre) const;
		string ConvertBooleanSymbols(const string& text) const;
		bool HasRequirements(lua_State* L, int unitDefID);
		bool IsBetter(lua_State* L, int thisDefID, int thatDefID);

	private:
		class UnitDefHolder {
			public:
				bool operator<(const UnitDefHolder&) const;
			public:
				int udID;
				const UnitDef* ud;
		};

	friend class CKeyAutoBinder::UnitDefHolder;
};


#endif /* KEY_AUTO_BINDER_H */
