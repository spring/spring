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
		float3 GetDTBuildSite(int uid);

        
        bool IsDragonsTeeth(const int Feature);
        bool IsDragonsTeeth(const char *FeatureName);
        bool IsDragonsTeeth(const std::string FeatureName);
        
        Global* G;
        std::map<int, int> DTBuilds;
        //float BuildPower;

        struct DTRing{
        	float3 Center;
        	std::vector<float3> DTPos;
        };
        std::vector<DTRing> DTRings;
        
        static bool NoDT;
};
