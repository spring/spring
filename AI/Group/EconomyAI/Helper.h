#include "MetalMap.h"

struct UnitDef;

class CHelper
{
	public:
		CHelper(IAICallback* aicb);
		virtual ~CHelper();

		pair<int,int> BuildNameToId(string name, int unit);
		string	BuildIdToName(int id, int unit);
		float3	FindBuildPos(string name, bool isMex, bool isGeo, int builder);
		void	DrawBuildArea();
		void	NewLocation(float3 centerPos, float radius, bool reset);
		void	AssignMetalMakerAI();
		void	SendTxt(const char *fmt, ...);
		void	ParseBuildOptions(map<string,const UnitDef*> &targetBO, const UnitDef* unitDef, bool recursive);

		float3	errorPos;
		CMetalMap* metalMap;
		int* friendlyUnits;
		float extractorRadius;
		float mmkrME;						// metalmaker M / E ratio
	private:
		bool	IsMetalSpotAvailable(float3 spot,float extraction);
		int		FindMetalSpots(float3 pos, float radius, vector<float3>* mexSpots);

		IAICallback* aicb;
		struct partition
		{
			float3 pos;
			string name;
			bool taken;
			bool empty;
		};
		struct location
		{
			float3 centerPos;
			float radius;
			float partitionRadius;
			int numPartitions;
			vector<float3> mexSpots;
			vector<partition> partitions;
		};
		vector<location*> locations;
		float maxPartitionRadius;
		int metalMakerAIid;
		const UnitDef* geoDef;
};

struct BOInfo
{
	string name;

	float mp;	// metal production
	float ep;	// energy production
	float me;	// metal production per energy upkeep
	float em;	// energy production per metal upkeep

	bool isMex;
	bool isGeo;

	float metalCost;
	float energyCost;
	float totalCost;
	float buildTime;
};

struct compareMetal
{
	bool operator()(BOInfo* const& bo1, BOInfo* const& bo2)
	{
		bool sameMetal	= max(bo1->mp,bo2->mp) / min(bo1->mp,bo2->mp) < 3 ? true : false;
		bool sameCost	= max(bo1->totalCost,bo2->totalCost) / min(bo1->totalCost,bo2->totalCost) < 10 ? true : false;

		if( sameMetal &&  sameCost) return bo1->me > bo2->me;
		if(!sameMetal &&  sameCost) return bo1->mp > bo2->mp;
		if( sameMetal && !sameCost) return (bo1->me / bo1->totalCost) > (bo2->me / bo2->totalCost);
		if(!sameMetal && !sameCost) return (bo1->mp / bo1->totalCost) > (bo2->mp / bo2->totalCost);
	}
};
struct compareEnergy
{
	bool operator()(BOInfo* const& bo1, BOInfo* const& bo2)
	{
		bool sameEnergy	= max(bo1->ep,bo2->ep) / min(bo1->ep,bo2->ep) < 3 ? true : false;
		bool sameCost	= max(bo1->totalCost,bo2->totalCost) / min(bo1->totalCost,bo2->totalCost) < 10 ? true : false;

		if( sameEnergy &&  sameCost) return bo1->em > bo2->em;
		if(!sameEnergy &&  sameCost) return bo1->ep > bo2->ep;
		if( sameEnergy && !sameCost) return (bo1->em / bo1->totalCost) > (bo2->em / bo2->totalCost);
		if(!sameEnergy && !sameCost) return (bo1->ep / bo1->totalCost) > (bo2->ep / bo2->totalCost);
	}
};
