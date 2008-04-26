struct UnitDef;

struct BOInfo
{
	std::string name;

	float mp;	// metal production
	float ep;	// energy production
	float me;	// metal production per energy upkeep
	float em;	// energy production per metal upkeep

	bool isMex;
	bool isGeo;

	float spacing;

	float metalCost;
	float energyCost;
	float totalCost;
	float buildTime;
};

struct compareMetal
{
	bool operator()(BOInfo* const& bo1, BOInfo* const& bo2)
	{
		bool sameMetal	= std::max(bo1->mp,bo2->mp) / std::min(bo1->mp,bo2->mp) < 3 ? true : false;
		bool sameCost	= std::max(bo1->totalCost, bo2->totalCost) / std::min(bo1->totalCost,bo2->totalCost) < 10 ? true : false;

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
		bool sameEnergy	= std::max(bo1->ep,bo2->ep) / std::min(bo1->ep,bo2->ep) < 3 ? true : false;
		bool sameCost	= std::max(bo1->totalCost, bo2->totalCost) / std::min(bo1->totalCost,bo2->totalCost) < 10 ? true : false;

		if( sameEnergy &&  sameCost) return bo1->em > bo2->em;
		if(!sameEnergy &&  sameCost) return bo1->ep > bo2->ep;
		if( sameEnergy && !sameCost) return (bo1->em / bo1->totalCost) > (bo2->em / bo2->totalCost);
		if(!sameEnergy && !sameCost) return (bo1->ep / bo1->totalCost) > (bo2->ep / bo2->totalCost);
	}
};


class CBoHandler
{
	public:
		CBoHandler(IAICallback* aicb,float mmkrME,float avgMetal,float maxPartitionRadius);
		virtual ~CBoHandler();

		void ClearBuildOptions();
		void AddBuildOptions(const UnitDef* unitDef);
		void SortBuildOptions();

		std::map<std::string,BOInfo*> allBO;
		std::vector<BOInfo*> bestMetal;	// ordered buildoptions for best metal production
		std::vector<BOInfo*> bestEnergy;	// ordered buildoptions for best energy production
	private:
		IAICallback* aicb;
		bool BOchanged;

		float mmkrME;

		float tidalStrength;
		float avgWind;
		float avgMetal;
		float maxPartitionRadius;
};
