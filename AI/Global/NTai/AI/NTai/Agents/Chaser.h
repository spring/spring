#ifndef CHASER_H
#define CHASER_H

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// The chaser class, deals with attacking and defending.

namespace ntai {
	class Chaser{
	public:
		Chaser();
		virtual ~Chaser();
		void InitAI(Global* GLI);
		void Add(int unit,bool aircraft=false);
		void Beacon(float3 pos, float radius);
		void UnitDestroyed(int unit,int attacker);
		void UnitFinished(int unit);
		void EnemyDestroyed(int enemy, int attacker);
		bool FindTarget(set<int> atkgroup, bool upthresh=true);
		void UnitDamaged(int damaged,int attacker,float damage,float3 dir);
		void UnitIdle(int unit);

		void Update();
		void FireSilos();
		void MakeTGA();
		void UpdateMatrixFriendlyUnits();
		void UpdateMatrixEnemies();

		void DoUnitStuff(int aa);
		set<int> engaged;
		set<int> walking;

		int threshold;
		float thresh_increase;
		float thresh_percentage_incr;
		int max_threshold;
		set<int> Attackers;
		set<int> sweap;

		float3 swtarget;


		Global* G;

		/**
		 * used for storing attackers while forming a group
		 */
		set<int> temp_attack_units;

		/**
		 * used for storing air attackers while forming a group
		 */
		set<int> temp_air_attack_units;
		
		vector<set<int> > attack_groups;
		vector<set<int> > air_attack_groups;

		/**
		 * a set of attackers that have been finished, but are still leaving the factory
		 */
		set<int> unit_to_initialize;	
		
		int enemynum;
		vector<string> hold_pos;
		vector<string> maneouvre;
		vector<string> roam;
		vector<string> hold_fire;
		vector<string> return_fire;
		vector<string> fire_at_will;

		CGridManager Grid;

	};
}
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
#endif

