#include "../Core/include.h"

namespace ntai {
	CConfigData::CConfigData(Global* G){
		_abstract = true;
		gaia = false;
		spacemod = false;
		dynamic_selection = true;
		use_modabstracts = false;
		absent_abstract = false;
		fire_state_commanders = 0;
		move_state_commanders = 0;
		scout_speed = 60;
		rule_extreme_interpolate= true;
		mod_tdf = new TdfParser(G);
	}

	CConfigData::~CConfigData(){
		//
		delete mod_tdf;
	}

	void CConfigData::Load(){
		//
		mod_tdf->GetDef(_abstract, "1", "AI\\abstract");
		mod_tdf->GetDef(gaia, "0", "AI\\GAIA");
		mod_tdf->GetDef(spacemod, "0", "AI\\spacemod");
		mod_tdf->GetDef(mexfirst, "0", "AI\\first_attack_mexraid");
		mod_tdf->GetDef(hardtarget, "0", "AI\\hard_target");
		mod_tdf->GetDef(mexscouting, "1", "AI\\mexscouting");
		mod_tdf->GetDef(dynamic_selection, "1", "AI\\dynamic_selection");
		mod_tdf->GetDef(use_modabstracts, "0", "AI\\use_mod_default");
		mod_tdf->GetDef(absent_abstract, "1", "AI\\use_mod_default_if_absent");
		mod_tdf->GetDef(rule_extreme_interpolate, "1", "AI\\rule_extreme_interpolate");
		antistall = atoi(mod_tdf->SGetValueDef("4", "AI\\antistall").c_str());
		Max_Stall_TimeMobile = (float)atof(mod_tdf->SGetValueDef("0", "AI\\MaxStallTime").c_str());
		Max_Stall_TimeIMMobile = (float)atof(mod_tdf->SGetValueDef(mod_tdf->SGetValueDef("0", "AI\\MaxStallTime"), "AI\\MaxStallTimeimmobile").c_str());

		fire_state_commanders = atoi(mod_tdf->SGetValueDef("0", "AI\\fire_state_commanders").c_str());
		move_state_commanders = atoi(mod_tdf->SGetValueDef("0", "AI\\move_state_commanders").c_str());
		scout_speed = (float)atof(mod_tdf->SGetValueDef("50", "AI\\scout_speed").c_str());

		if(_abstract == true){
			dynamic_selection = true;
		}

		if(use_modabstracts == true){
			_abstract = false;
		}
	    
	}
}
