#ifndef DGUN_CONTROLLER_HPP
#define DGUN_CONTROLLER_HPP


#include "Include.h"

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
		bool inRange(float3, float3, float);
		void setFireState(int);

		IAICallback* gAICallback;
		const UnitDef* commanderUD;
		int* units;

		int commanderID;
		int targetID;
		bool hasDGunOrder;
		bool hasReclaimOrder;
		bool hasRetreatOrder;
		unsigned int orderFrame;
		float3 startingPos;
};


#endif
