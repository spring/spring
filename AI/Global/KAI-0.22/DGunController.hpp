#ifndef DGUN_CONTROLLER_HPP
#define DGUN_CONTROLLER_HPP


#include "Include.h"

#undef CALLBACK
#define CALLBACK				(this -> gAICallback)
#define DGUN_MIN_HEALTH_RATIO	0.25
#define DGUN_MIN_ENERGY_LEVEL	380
#define DGUN_MAX_DAMAGE_LEVEL	250
#define FRAMERATE				 30


class DGunController {
	public:
		DGunController(void);
		~DGunController(void);

		void init(IAICallback*, int);
		void handleAttackEvent(int, float, float3, float3);
		void handleDestroyEvent(int, int);
		void update(unsigned int);

	private:
		void evadeIncomingFire(float3, float3, int);
		void issueOrder(int, int, unsigned int, int);
		void issueOrder(float3, int, unsigned int, int);
		void clearOrders(unsigned int);
		void setFireState(int);
		bool inRange(float3, float3, float);

		IAICallback* gAICallback;
		const UnitDef* commanderUD;
		int* units;

		bool inited;
		int commanderID;
		int targetID;
		bool hasDGunOrder;
		bool hasReclaimOrder;
		bool hasRetreatOrder;
		unsigned int dgunOrderFrame;
		unsigned int reclaimOrderFrame;
		unsigned int retreatOrderFrame;
		float3 startingPos;
};


#endif
