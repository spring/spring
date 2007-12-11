#ifndef DGUN_CONTROLLER_HPP
#define DGUN_CONTROLLER_HPP

#include "Include.h"

#define FRAMERATE 30
#define CALLOUT (this->gAICallback)

#define DGUN_MIN_HEALTH_RATIO	0.25
#define DGUN_MIN_ENERGY_LEVEL	(this->commanderWD->energycost)
#define DGUN_MAX_DAMAGE_LEVEL	(this->commanderUD->health / 4)
#define DGUN_MIN_METAL_VALUE	500


struct ControllerState {
	CR_DECLARE_STRUCT(ControllerState);

	ControllerState(void) {
		inited					= false;
		targetID				= -1;
		dgunOrderFrame			= 0;
		reclaimOrderFrame		= 0;
		targetSelectionFrame	= 0;
	}

	void reset(unsigned int currentFrame, bool clearNow) {
		if (clearNow) {
			dgunOrderFrame = 0;
			reclaimOrderFrame = 0;
			targetID = -1;
		} else {
			if (dgunOrderFrame > 0 && (currentFrame - dgunOrderFrame) > (FRAMERATE >> 0)) {
				// one second since last dgun order given, mark as expired
				dgunOrderFrame = 0;
				targetID = -1;
			}
			if (reclaimOrderFrame > 0 && (currentFrame - reclaimOrderFrame) > (FRAMERATE << 2)) {
				// four seconds since last reclaim order given, mark as expired
				reclaimOrderFrame = 0;
				targetID = -1;
			}
		}
	}

	void print(unsigned int currentFrame) {
		printf("CURRENT FRAME:        %d\n", currentFrame);
		printf("dgunOrderFrame:       %d\n", dgunOrderFrame);
		printf("reclaimOrderFrame:    %d\n", reclaimOrderFrame);
		printf("targetSelectionFrame: %d\n", targetSelectionFrame);
		printf("targetID:             %d\n", targetID);
		printf("\n");
	}

	bool inited;
	unsigned int dgunOrderFrame;
	unsigned int reclaimOrderFrame;
	unsigned int targetSelectionFrame;
	int targetID;
	float3 oldTargetPos;
};


class DGunController {
	public:
		CR_DECLARE(DGunController);
		void PostLoad();

		DGunController(AIClasses*);
		~DGunController(void);

		void init(int);
		void handleDestroyEvent(int, int);
		void update(unsigned int);
		void stop(void);
		bool isBusy(void);

	private:
		void trackAttackTarget(unsigned int);
		void selectTarget(unsigned int);

		void issueOrder(int, int, int);
		void issueOrder(float3, int, int);
		void setFireState(int);

		IAICallback* gAICallback;
		AIClasses* ai;
		const UnitDef* commanderUD;
		const WeaponDef* commanderWD;
		int* units;
		ControllerState state;
		int commanderID;
};


#endif
