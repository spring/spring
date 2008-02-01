// Actions

namespace ntai{

	class CActions {
	public:
		CActions(Global* GL);
		~CActions();

		void Update(); // check points of interest in LOS and remove accordingly

		bool RandomSpiral(int unit,bool water =false); // send the unit off in a random direction

		bool SeekOutInterest(int unit, float range=1000.0f); // send a unit out to look for points of interest

		bool SeekOutNearestInterest(int unit);

		bool AddPoint(float3 pos); // adds a point of interest

		bool Attack(int uid, float3 pos);
		bool Attack(int unit, int target,bool mobile);

		bool AttackNear(int unit,float LOSmultiplier=3.0f); // Attack nearby enemies...

		bool AttackNearest(int unit); // Attack nearby enemies...

		bool DGunNearby(int uid); // Makes the unit fire dgun at nearby objects if it has a dgun type weapon

		bool DGun(int uid,int enemy); // Makes 'uid' DGun 'enemy'

		bool Capture(int uid,int enemy); // Makesunit a capture unit b

		bool Repair(int uid,int unit); // Makes unit a repair unit b

		bool Move(int unit,float3 pos,bool overwrite=true); // makes the unit move to that position

		bool MoveToStrike(int unit,float3 pos,bool overwrite=true);
		// makes the unit move to that position but keepign its range as if its the position of an enemy

		bool Guard(int unit,int guarded); // Makes the unit guard the other unit

		bool Reclaim(int uid,int enemy); // reclaim a unti or feature

		bool AreaReclaim(int uid, float3 pos, int radius);
		bool Retreat(int uid);
		bool SeekOutLastAssault(int unit);
		bool Trajectory(int unit,int traj);
		void UnitDamaged(int damaged,int attacker,float damage,float3 dir);

		bool CopyAttack(int unit,set<int> tocopy);
		bool CopyMove(int unit,set<int> tocopy);

		bool IfNobodyNearMoveToNearest(int uid, set<int> units);

		vector<float3> points;

		/* This unit needs it's UnitIdle() called soon so schedule a unitIdle command */
		void ScheduleIdle(int unit);

		/* repair a nearby unit or unfinished building */
		bool RepairNearby(int uid,float radius); 

		/* repair a nearby unfinished unit */
		bool RepairNearbyUnfinishedMobileUnits(int uid,float radius); 

		/* repair a nearby unit based on offensive/defensive advantage against enemy */
		bool OffensiveRepairRetreat(int uid,float radius); 
		
		/* reclaim the nearest feature within a 700 pixel radius */
		bool ReclaimNearby(int uid, float radius); 

		/* issues an area resurrect if there are any features in a vicinity of 700 ticks */
		bool RessurectNearby(int uid);

	private:
		Global* G;
		float3 last_attack;

		int* temp;
	};
}

