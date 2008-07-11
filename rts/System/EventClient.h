#ifndef EVENT_CLIENT_H
#define EVENT_CLIENT_H
// EventClient.h: interface for the CEventClient class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map;

class CUnit;
class CWeapon;
class CFeature;
class CProjectile;
struct Command;


class CEventClient
{
	public:
		enum SpecialTeams {
			NoAccessTeam   = -1,
			AllAccessTeam  = -2,
			MinSpecialTeam = AllAccessTeam 
		};

	public:
		inline const std::string& GetName()   const { return name;   }
		inline int                GetOrder()  const { return order;  }
		inline bool               GetSynced() const { return synced; }

		// used by the eventHandler to register
		// call-ins when an EventClient is being added
		virtual bool WantsEvent(const std::string& eventName) { return false; }

		// used by the eventHandler to route certain event types
		virtual int  GetReadAllyTeam() const { return NoAccessTeam; }
		virtual bool GetFullRead()     const { return GetReadAllyTeam() == AllAccessTeam; }
		inline bool CanReadAllyTeam(int allyTeam) {
			return GetFullRead() || (GetReadAllyTeam() == allyTeam);
		}

	private:
		const std::string name;
		const int         order;
		const bool        synced;

	protected:
		CEventClient(const std::string& name, int order, bool synced);
		virtual ~CEventClient();

	public:
		// Synced
		virtual void GamePreload();
		virtual void GameStart();
		virtual void GameOver();
		virtual void TeamDied(int teamID);
		virtual void TeamChanged(int teamID);
		virtual void PlayerChanged(int playerID);
		virtual void PlayerRemoved(int playerID);

		virtual void UnitCreated(const CUnit* unit, const CUnit* builder);
		virtual void UnitFinished(const CUnit* unit);
		virtual void UnitFromFactory(const CUnit* unit, const CUnit* factory,
		                             bool userOrders);
		virtual void UnitDestroyed(const CUnit* unit, const CUnit* attacker);
		virtual void UnitTaken(const CUnit* unit, int newTeam);
		virtual void UnitGiven(const CUnit* unit, int oldTeam);

		virtual void UnitIdle(const CUnit* unit);
		virtual void UnitCommand(const CUnit* unit, const Command& command);
		virtual void UnitCmdDone(const CUnit* unit, int cmdType, int cmdTag);
		virtual void UnitDamaged(const CUnit* unit, const CUnit* attacker,
		                         float damage, int weaponID, bool paralyzer);
		virtual void UnitExperience(const CUnit* unit, float oldExperience);

		virtual void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                             const float3& pos, float strength);
		virtual void UnitEnteredRadar(const CUnit* unit, int allyTeam);
		virtual void UnitEnteredLos(const CUnit* unit, int allyTeam);
		virtual void UnitLeftRadar(const CUnit* unit, int allyTeam);
		virtual void UnitLeftLos(const CUnit* unit, int allyTeam);

		virtual void UnitEnteredWater(const CUnit* unit);
		virtual void UnitEnteredAir(const CUnit* unit);
		virtual void UnitLeftWater(const CUnit* unit);
		virtual void UnitLeftAir(const CUnit* unit);

		virtual void UnitLoaded(const CUnit* unit, const CUnit* transport);
		virtual void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		virtual void UnitCloaked(const CUnit* unit);
		virtual void UnitDecloaked(const CUnit* unit);

		virtual void FeatureCreated(const CFeature* feature);
		virtual void FeatureDestroyed(const CFeature* feature);

		virtual void ProjectileCreated(const CProjectile* proj);
		virtual void ProjectileDestroyed(const CProjectile* proj);

		virtual void StockpileChanged(const CUnit* unit,
		                              const CWeapon* weapon, int oldCount);
	
		virtual bool Explosion(int weaponID, const float3& pos, const CUnit* owner);

		// Unsynced
		virtual void Update();

		virtual void ViewResize();

		virtual bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		virtual void DrawGenesis();
		virtual void DrawWorld();
		virtual void DrawWorldPreUnit();
		virtual void DrawWorldShadow();
		virtual void DrawWorldReflection();
		virtual void DrawWorldRefraction();
		virtual void DrawScreenEffects();
		virtual void DrawScreen();
		virtual void DrawInMiniMap();

		// from LuaUI
		virtual bool KeyPress(unsigned short key, bool isRepeat);
		virtual bool KeyRelease(unsigned short key);
		virtual bool MouseMove(int x, int y, int dx, int dy, int button);
		virtual bool MousePress(int x, int y, int button);
		virtual int  MouseRelease(int x, int y, int button); // FIXME - bool / void?
		virtual bool MouseWheel(bool up, float value);
		virtual bool IsAbove(int x, int y);
		virtual std::string GetTooltip(int x, int y);

		virtual bool CommandNotify(const Command& cmd);

		virtual bool AddConsoleLine(const std::string& msg, int zone);

		virtual bool GroupChanged(int groupID);

		virtual bool GameSetup(const std::string& state, bool& ready,
		                       const map<int, std::string>& playerStates);

		virtual std::string WorldTooltip(const CUnit* unit,
		                                 const CFeature* feature,
		                                 const float3* groundPos);

		virtual bool MapDrawCmd(int playerID, int type,
		                        const float3* pos0,
		                        const float3* pos1,
		                        const std::string* label);
};


#endif /* EVENT_CLIENT_H */
