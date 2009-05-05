#ifndef KAIK_DGUNCONTROLLER_HDR
#define KAIK_DGUNCONTROLLER_HDR

#include <list>
#include <map>

#define FRAMERATE 30

#define DGUN_MIN_HEALTH_RATIO	0.25
#define DGUN_MIN_ENERGY_LEVEL	(this->commanderWD->energycost)
#define DGUN_MAX_DAMAGE_LEVEL	(this->commanderUD->health / 4)
#define DGUN_MIN_METAL_VALUE	500

class IAICallback;
struct UnitDef;
struct WeaponDef;
struct AIClasses;

struct ControllerState {
	CR_DECLARE_STRUCT(ControllerState);

	ControllerState(void) {
		inited              = false;
		targetID             = -1;
		dgunOrderFrame       =  0;
		reclaimOrderFrame    =  0;
		captureOrderFrame    =  0;
		targetSelectionFrame =  0;
	}

	void Reset(unsigned int currentFrame, bool clearNow) {
		if (clearNow) {
			dgunOrderFrame    =  0;
			reclaimOrderFrame =  0;
			captureOrderFrame =  0;
			targetID          = -1;
		} else {
			if (dgunOrderFrame > 0 && (currentFrame - dgunOrderFrame) > (FRAMERATE >> 0)) {
				// one second since last dgun order, mark as expired
				dgunOrderFrame =  0;
				targetID       = -1;
			}
			if (reclaimOrderFrame > 0 && (currentFrame - reclaimOrderFrame) > (FRAMERATE << 2)) {
				// four seconds since last reclaim order, mark as expired
				reclaimOrderFrame =  0;
				targetID          = -1;
			}
			if (captureOrderFrame > 0 && (currentFrame - captureOrderFrame) > (FRAMERATE << 3)) {
				// eight seconds since last capture order, mark as expired
				captureOrderFrame =  0;
				targetID          = -1;
			}
		}
	}

	bool   inited;
	int    dgunOrderFrame;
	int    reclaimOrderFrame;
	int    captureOrderFrame;
	int    targetSelectionFrame;
	int    targetID;
	float3 oldTargetPos;
};

class CDGunController {
	public:
		CR_DECLARE(CDGunController);

		CDGunController(AIClasses*);
		~CDGunController(void) {}
		void PostLoad();

		void Init(int);
		void HandleEnemyDestroyed(int, int);
		void Update(unsigned int);
		void Stop(void) const;
		bool IsBusy(void) const;

	private:
		void TrackAttackTarget(unsigned int);
		void SelectTarget(unsigned int);

		void IssueOrder(int, int, int) const;
		void IssueOrder(const float3&, int, int) const;

		AIClasses* ai;
		const UnitDef* commanderUD;
		const WeaponDef* commanderWD;
		ControllerState state;
		int commanderID;
};

class CDGunControllerHandler {
	public:
		CR_DECLARE(CDGunControllerHandler);

		CDGunControllerHandler(AIClasses* aic) {
			ai = aic;
		}
		void PostLoad();

		bool AddController(int unitID);
		bool DelController(int unitID);
		CDGunController* GetController(int unitID) const;

		void NotifyEnemyDestroyed(int, int);
		void Update(int frame);

	private:
		std::map<int, CDGunController*> controllers;
		AIClasses* ai;
};

#endif
