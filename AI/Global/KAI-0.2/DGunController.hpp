#ifndef DGUN_CONTROLLER_HPP
#define DGUN_CONTROLLER_HPP


#include "Include.h"

#undef CALLBACK
#define CALLBACK				(this -> gAICallback)
#define DGUN_MIN_HEALTH_RATIO	0.25
#define DGUN_MIN_ENERGY_LEVEL	(this -> commanderDGun -> energycost)
#define DGUN_MAX_DAMAGE_LEVEL	(this -> commanderUD -> health/4)
//#define FRAMERATE				 30
#define ORDERS_TIMEOUT          30*4


class DGunController {
	CR_DECLARE(DGunController);
	public:
		DGunController(AIClasses* ai);
		~DGunController(void);
		void PostLoad();

		void init(IAICallback*, int);
		void handleAttackEvent(int, float, float3, float3);
		void handleDestroyEvent(int, int);
		void update(unsigned int);
		void clearBuild();

		bool inited;
		float3 startingPos;
	private:
		void evadeIncomingFire(float3, float3, int);
		void issueOrder(int, int, unsigned int, int);
		void issueOrder(float3, int, unsigned int, int);
		void clearOrders(unsigned int);
		void setFireState(int);
		bool inRange(float3, float3, float);

		IAICallback* gAICallback;
		AIClasses *ai;
		const UnitDef* commanderUD;
		const WeaponDef* commanderDGun;
		int* units;

		int commanderID;
		int targetID;
		bool hasDGunOrder;
		bool hasReclaimOrder;
		bool hasRetreatOrder;
		unsigned int dgunOrderFrame;
		unsigned int reclaimOrderFrame;
		unsigned int retreatOrderFrame;
		BuilderTracker *bt;
};


#endif
