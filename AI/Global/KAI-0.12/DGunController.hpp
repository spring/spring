#ifndef DGUN_CONTROLLER_HPP
#define DGUN_CONTROLLER_HPP


#include "Include.h"

#define CALLBACK (this -> gAICallback)

#define DGUN_MIN_ENERGY_LEVEL	380
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

		bool inRange(float3, float3);
		void setFireState(int);

		IAICallback* gAICallback;

		int commanderID;
		int targetID;
		bool hasDGunOrder;
		bool hasReclaimOrder;
		unsigned int dgunOrderFrame;
		unsigned int reclaimOrderFrame;

		int* units;
};


#endif
