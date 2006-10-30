#ifndef WAIT_COMMANDS_AI_H
#define WAIT_COMMANDS_AI_H
// WaitCommandsAI.h: interface for the CWaitCommandsAI class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <map>
#include <set>
using namespace std;

#include "System/Object.h"

class float3;
class CUnit;
class CObject;
struct Command;


class CWaitCommandsAI {
	public:
		CWaitCommandsAI();
		~CWaitCommandsAI();

		void Update();
		void DrawCommands();

		// called from SelectedUnits
		void AddTimeWait(const Command& cmd);
		void AddDeathWatch(const Command& cmd);
		void AddRallyPoint(const Command& cmd);

		static const float timeWaitCode;
		static const float deathWatchCode;
		static const float rallyPointCode;
		
	private:
		class Wait;
		void DeleteWaitObject(Wait*);
		

	private:
		typedef set<CUnit*> UnitSet;

		// Wait Base Class
		class Wait : public CObject {
			public:
				virtual ~Wait();
				virtual void DependentDied(CObject* o) = 0; // from CObject
				virtual void Update() = 0;
				virtual void Draw() const = 0;
			protected:
				enum WaitState {
					Active,
					Queued,
					Missing
				};
				WaitState GetWaitState(const CUnit* unit,
				                       float code, float key, int frame) const;
				bool IsWaitingOn(const CUnit* unit, float code, float key) const;
				void SendWaitCommand(const UnitSet& unitSet);
				UnitSet::iterator RemoveUnit(UnitSet::iterator, UnitSet&);
			protected:
				float key;
				int deadFrame;
				bool deleteMe;
		};
		typedef set<Wait*> WaitSet;
		WaitSet waitSet;

		// TimeWait				
		class TimeWait : public Wait {
			public:
				static TimeWait* New(const Command& cmd, CUnit* unit);
				~TimeWait();
				void DependentDied(CObject* o);
				void Update();
				void Draw() const;
			private:
				TimeWait(const Command& cmd, CUnit* unit);
				CUnit* unit;
				bool enabled;
				int duration;
				int endFrame;
				static float keySource;
				static map<CUnit*, TimeWait*> lookup;
		};

		// DeathWatch
		class DeathWatch : public Wait {
			public:
				static DeathWatch* New(const Command& cmd);
				~DeathWatch();
				void DependentDied(CObject* o);
				void Update();
				void Draw() const;
			private:
				DeathWatch(const Command& cmd);
				void SelectAreaUnits(const float3& pos0, const float3& pos1,
				                     UnitSet& units, bool enemies);
			private:
				float key;
				UnitSet waitUnits;
				UnitSet deathUnits;
				static float keySource;
		};

		// RallyPoint
		class RallyPoint : public Wait {
			public:
				static RallyPoint* New(const Command& cmd);
				~RallyPoint();
				void DependentDied(CObject * o);
				void Update();
				void Draw() const;
			private:
				RallyPoint(const Command& cmd);
				float key;
				UnitSet waitUnits;
				static float keySource;
		};

	private:
		WaitSet::iterator RemoveWait(WaitSet::iterator, WaitSet&);
};


extern CWaitCommandsAI waitCommandsAI;


#endif /* WAIT_COMMANDS_AI_H */
