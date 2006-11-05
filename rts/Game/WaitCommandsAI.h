#ifndef WAIT_COMMANDS_AI_H
#define WAIT_COMMANDS_AI_H
// WaitCommandsAI.h: interface for the CWaitCommandsAI class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <time.h>
#include <map>
#include <set>
#include <deque>
#include <string>
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
		void DrawCommands() const;

		// called from SelectedUnits
		void AddTimeWait(const Command& cmd);
		void AddDeathWait(const Command& cmd);
		void AddSquadWait(const Command& cmd);
		void AddGatherWait(const Command& cmd);

		void AcknowledgeCommand(const Command& cmd);

		void ClearUnitQueue(CUnit* unit, const deque<Command>& queue);
		void RemoveWaitCommand(CUnit* unit, const Command& cmd);
		
		void NewUnit(CUnit* unit, const CUnit* builder);
		
		void AddIcon(const Command& cmd, const float3& pos) const;

	private:
		class Wait;
		bool InsertWaitObject(Wait*);
		void RemoveWaitObject(Wait*);

	private:
		typedef int KeyType;
		typedef set<CUnit*> UnitSet;

		typedef map<KeyType, Wait*> WaitMap;
		WaitMap waitMap;
		WaitMap unackedMap;
		
	private:
		// Wait Base Class
		class Wait : public CObject {
			public:
				virtual ~Wait();
				virtual void DependentDied(CObject* o) = 0; // from CObject
				virtual void AddUnit(CUnit* unit) = 0;
				virtual void RemoveUnit(CUnit* unit) = 0;
				virtual void Update() = 0;
				virtual void Draw() const { return; }
				virtual const string& GetStateText() const { return noText; }
			public:
				time_t GetDeadTime() const { return deadTime; }
				float GetCode() const { return code; }
				KeyType GetKey() const { return key; }
			public:
				static KeyType GetKeyFromFloat(float f);
				static float GetFloatFromKey(KeyType k);
			protected:
				Wait(float code);
				enum WaitState {
					Active, Queued, Missing
				};
				WaitState GetWaitState(const CUnit* unit) const;
				bool IsWaitingOn(const CUnit* unit) const;
				void SendCommand(const Command& cmd, const UnitSet& unitSet);
				void SendWaitCommand(const UnitSet& unitSet);
				UnitSet::iterator RemoveUnitFromSet(UnitSet::iterator, UnitSet&);
			protected:
				const float code;
				KeyType key;
				bool valid;
				time_t deadTime;
			protected:
				static KeyType GetNewKey();
			private:
				static KeyType keySource;
				static const string noText;
		};

		// TimeWait				
		class TimeWait : public Wait {
			public:
				static TimeWait* New(const Command& cmd, CUnit* unit);
				static TimeWait* New(int duration, CUnit* unit);
				~TimeWait();
				void DependentDied(CObject* o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
				void Draw() const;
				const string& GetStateText() const;
				int GetDuration() const { return duration; }
			private:
				TimeWait(const Command& cmd, CUnit* unit);
				TimeWait(int duration, CUnit* unit);
			private:
				CUnit* unit;
				bool enabled;
				int duration;
				int endFrame;
				bool factory;
		};

		// DeathWait
		class DeathWait : public Wait {
			public:
				static DeathWait* New(const Command& cmd);
				~DeathWait();
				void DependentDied(CObject* o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
				void Draw() const;
			private:
				DeathWait(const Command& cmd);
				void SelectAreaUnits(const float3& pos0, const float3& pos1,
				                     UnitSet& units, bool enemies);
			private:
				UnitSet waitUnits;
				UnitSet deathUnits;
		};

		// SquadWait				
		class SquadWait : public Wait {
			public:
				static SquadWait* New(const Command& cmd);
				~SquadWait();
				void DependentDied(CObject* o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
				void Draw() const;
				const string& GetStateText() const { return stateText; }
			private:
				SquadWait(const Command& cmd);
				void UpdateText();
			private:
				int squadCount;
				UnitSet buildUnits;
				UnitSet waitUnits;
				string stateText;
		};

		// GatherWait
		class GatherWait : public Wait {
			public:
				static GatherWait* New(const Command& cmd);
				~GatherWait();
				void DependentDied(CObject * o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
			private:
				GatherWait(const Command& cmd);
			private:
				UnitSet waitUnits;
		};
};


extern CWaitCommandsAI waitCommandsAI;


#endif /* WAIT_COMMANDS_AI_H */
