
namespace ntai {

	class CConfigData {
	public:
		CConfigData(Global* G);
		~CConfigData();

		void Load();

		bool _abstract;
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
}
