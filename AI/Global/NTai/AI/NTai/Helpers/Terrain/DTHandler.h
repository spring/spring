/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

/*
This was originally taken from OTAI and heavily modified
credits go to veylon the coder and maintainer of that AI.
*/

namespace ntai {

	// Dragon's Teeth
	#define DT_SIZE             32
	#define DT_NUM_GAPS         6
	#define DT_GAP_WIDTH        8
	#define DT_MAX_BUILD_POWER  200
	#define DT_DONE_WAIT_TIME   9000

	class Global;
	class CDTHandler{
		public:
			CDTHandler(Global* GL);
			~CDTHandler();
			void AddRing(float3 loc, float Radius, float Twist);
			bool DTNeeded();
			float3 GetDTBuildSite(float3 pos);

	//		bool IsDragonsTeeth(const int Feature);
			bool IsDragonsTeeth(const char *FeatureName);
			bool IsDragonsTeeth(const std::string FeatureName);
			bool IsDragonsTeeth(const UnitDef* ud);
			Global* G;
			std::map<int, int> DTBuilds;
			//float BuildPower;

			struct DTRing{
        		float3 Center;
        		std::vector<float3> DTPos;
			};
			std::vector<DTRing> DTRings;
	        
			bool NoDT;
	};

}
