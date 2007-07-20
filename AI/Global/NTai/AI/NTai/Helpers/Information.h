class CInfo {
public:
	CInfo(Global* G){
		abstract = true;
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
	bool abstract;
	bool mexfirst;
	bool mexscouting;
	bool gaia;
	bool hardtarget;
	bool spacemod;
	bool dynamic_selection;
	bool use_modabstracts;
	bool absent_abstract;
	float scout_speed;
	unsigned int fire_state_commanders;
	unsigned int move_state_commanders;
	int antistall;
	float Max_Stall_TimeMobile;
	float Max_Stall_TimeIMMobile;
	bool rule_extreme_interpolate;

	string tdfpath;
	string datapath;
	TdfParser* mod_tdf;
};
