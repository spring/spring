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

		std::pair<int,int> BuildNameToId(std::string name, int unit);
		std::string	BuildIdToName(int id, int unit);
		float3 FindBuildPos(std::string name, bool isMex, bool isGeo, float distance, int builder);
		void	DrawBuildArea();
		void	NewLocation(float3 centerPos, float radius);
		void	ResetLocations();
		void	AssignMetalMakerAI();
		void	SendTxt(const char *fmt, ...);
		void	ParseBuildOptions(std::map<std::string,const UnitDef*> &targetBO, const UnitDef* unitDef, bool recursive);

		float3	errorPos;
		CMetalMap* metalMap;
		std::vector<int> friendlyUnits;
		int myTeam;
		float extractorRadius;
		float mmkrME;						// metalmaker M / E ratio
		float maxPartitionRadius;
		IAICallback* aicb;
		CGroupAI *owner;
	private:
		bool	IsMetalSpotAvailable(float3 spot,float extraction);
		int		FindMetalSpots(float3 pos, float radius, std::vector<float3>* mexSpots);

		struct partition
		{
			CR_DECLARE_STRUCT(partition);
			float3 pos;
			std::string name;
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
			std::vector<float3> mexSpots;
			std::vector<partition> partitions;
		};
		std::vector<location*> locations;
		int metalMakerAIid;
		const UnitDef* geoDef;
		float drawColor[4];
};

#endif
