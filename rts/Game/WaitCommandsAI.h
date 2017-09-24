/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WAIT_COMMANDS_AI_H
#define WAIT_COMMANDS_AI_H

#include <deque>
#include <string>

#include "System/Object.h"
#include "System/Misc/SpringTime.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"

class float3;
class CObject;
class CUnit;
struct Command;
class CCommandQueue;

class CWaitCommandsAI {
	CR_DECLARE_STRUCT(CWaitCommandsAI)
	CR_DECLARE_SUB(Wait)
	CR_DECLARE_SUB(TimeWait)
	CR_DECLARE_SUB(DeathWait)
	CR_DECLARE_SUB(SquadWait)
	CR_DECLARE_SUB(GatherWait)

	public:
		typedef spring::unordered_set<int> CUnitSet;

		CWaitCommandsAI();
		~CWaitCommandsAI();

		void Update();
		void DrawCommands() const;

		// called from SelectedUnits
		void AddTimeWait(const Command& cmd);
		void AddDeathWait(const Command& cmd);
		void AddSquadWait(const Command& cmd);
		void AddGatherWait(const Command& cmd);

		/// acknowledge a command received from the network
		void AcknowledgeCommand(const Command& cmd);

		/// search a new unit's queue and add it to its wait commands
		void AddLocalUnit(CUnit* unit, const CUnit* builder);

		void ClearUnitQueue(CUnit* unit, const CCommandQueue& queue);
		void RemoveWaitCommand(CUnit* unit, const Command& cmd);

		void AddIcon(const Command& cmd, const float3& pos) const;

	private:
		class Wait;
		bool InsertWaitObject(Wait* wait);
		void RemoveWaitObject(Wait* wait);

	private:
		typedef int KeyType;
		typedef spring::unordered_map<KeyType, Wait*> WaitMap;
		WaitMap waitMap;
		WaitMap unackedMap;

	private:
		// Wait Base Class
		class Wait : public CObject {
			CR_DECLARE(Wait)
			public:
				virtual ~Wait();
				virtual void DependentDied(CObject* o) = 0; // from CObject
				virtual void AddUnit(CUnit* unit) = 0;
				virtual void RemoveUnit(CUnit* unit) = 0;
				virtual void Update() = 0;
				virtual void Draw() const {}
				virtual void AddUnitPosition(const float3& pos) {}
				virtual const std::string& GetStateText() const { return noText; }
			public:
				spring_time GetDeadTime() const { return deadTime; }
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
				void SendCommand(const Command& cmd, const CUnitSet& unitSet);
				void SendWaitCommand(const CUnitSet& unitSet);

			protected:
				float code;
				KeyType key;
				bool valid;
				spring_time deadTime;
			protected:
				static KeyType GetNewKey();
			private:
				static KeyType keySource;
				static const std::string noText;
				void PostLoad();
		};

		// TimeWait
		class TimeWait : public Wait {
			CR_DECLARE(TimeWait)
			public:
				static TimeWait* New(const Command& cmd, CUnit* unit);
				static TimeWait* New(int duration, CUnit* unit);
				~TimeWait();
				void DependentDied(CObject* o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
				void Draw() const;
				const std::string& GetStateText() const;
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
			CR_DECLARE(DeathWait)
			public:
				static DeathWait* New(const Command& cmd);
				~DeathWait();
				void DependentDied(CObject* o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
				void Draw() const;
				void AddUnitPosition(const float3& pos);
			private:
				DeathWait(const Command& cmd);
				void SelectAreaUnits(const float3& pos0, const float3& pos1,
				                     CUnitSet& units, bool enemies);
			private:
				CUnitSet waitUnits;
				CUnitSet deathUnits;
				std::vector<float3> unitPos;
		};

		// SquadWait
		class SquadWait : public Wait {
			CR_DECLARE(SquadWait)
			public:
				static SquadWait* New(const Command& cmd);
				~SquadWait();
				void DependentDied(CObject* o);
				void AddUnit(CUnit* unit);
				void RemoveUnit(CUnit* unit);
				void Update();
				void Draw() const;
				const std::string& GetStateText() const { return stateText; }
			private:
				SquadWait(const Command& cmd);
				void UpdateText();
			private:
				int squadCount;
				CUnitSet buildUnits;
				CUnitSet waitUnits;
				std::string stateText;
		};

		// GatherWait
		class GatherWait : public Wait {
			CR_DECLARE(GatherWait)
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
				CUnitSet waitUnits;
		};
};


extern CWaitCommandsAI waitCommandsAI;


#endif /* WAIT_COMMANDS_AI_H */
