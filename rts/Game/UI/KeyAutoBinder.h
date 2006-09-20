#ifndef KEY_AUTO_BINDER_H
#define KEY_AUTO_BINDER_H
// KeyAutoBinder.h: interface for the CKeyAutoBinder class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <vector>
#include <set>
using namespace std;


extern "C" {
	#include "lua.h"
}


struct UnitDef;


class CKeyAutoBinder {

	public:
		CKeyAutoBinder();
		~CKeyAutoBinder();
		
		bool BindBuildType(const string& keystr,
		                   const vector<string>& requirements,
		                   const vector<string>& sortCriteria,
		                   const vector<string>& chords);
		
	private:
		bool LoadCode(const string& code, const string& debug);
		bool LoadInfo();
		string MakeGameInfo();
		string MakeUnitDefInfo();
		string MakeWeaponDefInfo();
		string MakeRequirementCall(const vector<string>& requirements);
		string MakeSortCriteriaCall(const vector<string>& sortCriteria);
		string AddUnitDefPrefix(const string& text, const string& pre) const;
		string ConvertBooleanSymbols(const string& text) const;
		bool HasRequirements(int unitDefID);
		bool IsBetter(int thisDefID, int thatDefID);
		
	private:
		lua_State* L;
		
		set<string> unitDefParams;
		
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
