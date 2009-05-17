#ifndef KAIK_METALMAKER_HDR
#define KAIK_METALMAKER_HDR

#include <vector>

class IAICallback;
struct AIClasses;

class CMetalMaker {
	public:
		CR_DECLARE(CMetalMaker);
		CR_DECLARE_SUB(UnitInfo);

		CMetalMaker(AIClasses*);
		~CMetalMaker();
		void PostLoad();

		bool Add(int unit);
		bool Remove(int unit);
		bool AllAreOn();
		void Update(int);

		struct UnitInfo {
			CR_DECLARE_STRUCT(UnitInfo);

			int id;
			float energyUse;
			float metalPerEnergy;
			bool turnedOn;
		};

		std::vector<UnitInfo> myUnits;
		float lastEnergy;
		int listIndex;
		int addedDelay;

	private:
		AIClasses* ai;
};


#endif
