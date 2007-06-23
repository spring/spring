#ifndef ECONOMY_HELPER_H
#define ECONOMY_HELPER_H
#include "MetalMap.h"

struct UnitDef;
class CGroupAI;

class CHelper
{
	CR_DECLARE(CHelper);
	CR_DECLARE_SUB(partition);
	CR_DECLARE_SUB(location);
	public:
		CHelper(IAICallback* aicb,CGroupAI *owner);
		virtual ~CHelper();
		void PostLoad();

		pair<int,int> BuildNameToId(string name, int unit);
		string	BuildIdToName(int id, int unit);
		float3	FindBuildPos(string name, bool isMex, bool isGeo, float distance, int builder);
		void	DrawBuildArea();
		void	NewLocation(float3 centerPos, float radius);
		void	ResetLocations();
		void	AssignMetalMakerAI();
		void	SendTxt(const char *fmt, ...);
		void	ParseBuildOptions(map<string,const UnitDef*> &targetBO, const UnitDef* unitDef, bool recursive);

		float3	errorPos;
		CMetalMap* metalMap;
		vector<int> friendlyUnits;
		int myTeam;
		float extractorRadius;
		float mmkrME;						// metalmaker M / E ratio
		float maxPartitionRadius;
		IAICallback* aicb;
		CGroupAI *owner;
	private:
		bool	IsMetalSpotAvailable(float3 spot,float extraction);
		int		FindMetalSpots(float3 pos, float radius, vector<float3>* mexSpots);

		struct partition
		{
			CR_DECLARE_STRUCT(partition);
			float3 pos;
			string name;
			bool taken;
			bool empty;
		};
		struct location
		{
			CR_DECLARE_STRUCT(location);
			float3 centerPos;
			float radius;
			float partitionRadius;
			int numPartitions;
			int squarePartitions;
			vector<float3> mexSpots;
			vector<partition> partitions;
		};
		vector<location*> locations;
		int metalMakerAIid;
		const UnitDef* geoDef;
		float drawColor[4];
};

#endif
