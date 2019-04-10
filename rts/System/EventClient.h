/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EVENT_CLIENT_H
#define EVENT_CLIENT_H

#include <algorithm>
#include <typeinfo>
#include <string>
#include <vector>

#include "System/float3.h"
#include "System/Misc/SpringTime.h"

#ifdef __APPLE__
// defined in X11/X.h
#undef KeyPress
#undef KeyRelease
#endif

using std::string;
using std::vector;

class CSolidObject;
class CUnit;
class CWeapon;
class CFeature;
class CProjectile;
struct Command;
class IArchive;
struct SRectangle;
struct UnitDef;
struct BuildInfo;
struct FeatureDef;
class LuaMaterial;

#ifndef zipFile
	// might be defined through zip.h already
	typedef void* zipFile;
#endif


enum DbgTimingInfoType {
	TIMING_VIDEO,
	TIMING_SIM,
	TIMING_GC,
	TIMING_SWAP,
	TIMING_UNSYNCED
};


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
		inline bool               GetSynced() const { return synced_; }

		/**
		 * Used by the eventHandler to register
		 * call-ins when an EventClient is being added.
		 */
		virtual bool WantsEvent(const std::string& eventName);

		// used by the eventHandler to route certain event types
		virtual int  GetReadAllyTeam() const { return NoAccessTeam; }
		virtual bool GetFullRead()     const { return GetReadAllyTeam() == AllAccessTeam; }
		inline bool CanReadAllyTeam(int allyTeam) {
			return (GetFullRead() || (GetReadAllyTeam() == allyTeam));
		}

	protected:
		CEventClient(const std::string& name, int order, bool synced);
		virtual ~CEventClient();

	protected:
		const std::string name;
		const int         order;
		const bool        synced_;
		      bool        autoLinkEvents;

	protected:
		friend class CEventHandler;
		typedef std::pair<std::string, bool> LinkPair;

		std::vector<LinkPair> autoLinkedEvents;

		template <class T>
		void RegisterLinkedEvents(T* foo) {
			#define SETUP_EVENT(eventname, props) \
				autoLinkedEvents.push_back({#eventname, typeid(&T::eventname) != typeid(&CEventClient::eventname)});

				#include "Events.def"
			#undef SETUP_EVENT

			std::stable_sort(autoLinkedEvents.begin(), autoLinkedEvents.end(), [](const LinkPair& a, const LinkPair& b) { return (a.first < b.first); });
		}

	public:
		/**
		 * @name Synced_events
		 * @{
		 */
		virtual void Load(IArchive* archive) {}

		virtual void GamePreload() {}
		virtual void GameStart() {}
		virtual void GameOver(const std::vector<unsigned char>& winningAllyTeams) {}
		virtual void GamePaused(int playerID, bool paused) {}
		virtual void GameFrame(int gameFrame) {}
		virtual void GameID(const unsigned char* gameID, unsigned int numBytes) {}

		virtual void TeamDied(int teamID) {}
		virtual void TeamChanged(int teamID) {}
		virtual void PlayerChanged(int playerID) {}
		virtual void PlayerAdded(int playerID) {}
		virtual void PlayerRemoved(int playerID, int reason) {}

		virtual void UnitCreated(const CUnit* unit, const CUnit* builder) {}
		virtual void UnitFinished(const CUnit* unit) {}
		virtual void UnitReverseBuilt(const CUnit* unit) {}
		virtual void UnitFromFactory(const CUnit* unit, const CUnit* factory, bool userOrders) {}
		virtual void UnitDestroyed(const CUnit* unit, const CUnit* attacker) {}
		virtual void UnitTaken(const CUnit* unit, int oldTeam, int newTeam) {}
		virtual void UnitGiven(const CUnit* unit, int oldTeam, int newTeam) {}

		virtual void UnitIdle(const CUnit* unit) {}
		virtual void UnitCommand(const CUnit* unit, const Command& command) {}
		virtual void UnitCmdDone(const CUnit* unit, const Command& command) {}
		virtual void UnitDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer) {}
		virtual void UnitStunned(const CUnit* unit, bool stunned) {}
		virtual void UnitExperience(const CUnit* unit, float oldExperience) {}
		virtual void UnitHarvestStorageFull(const CUnit* unit) {}

		virtual void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                             const float3& pos, float strength) {}
		virtual void UnitEnteredRadar(const CUnit* unit, int allyTeam) {}
		virtual void UnitEnteredLos(const CUnit* unit, int allyTeam) {}
		virtual void UnitLeftRadar(const CUnit* unit, int allyTeam) {}
		virtual void UnitLeftLos(const CUnit* unit, int allyTeam) {}

		virtual void UnitEnteredWater(const CUnit* unit) {}
		virtual void UnitEnteredAir(const CUnit* unit) {}
		virtual void UnitLeftWater(const CUnit* unit) {}
		virtual void UnitLeftAir(const CUnit* unit) {}

		virtual void UnitLoaded(const CUnit* unit, const CUnit* transport) {}
		virtual void UnitUnloaded(const CUnit* unit, const CUnit* transport) {}

		virtual void UnitCloaked(const CUnit* unit) {}
		virtual void UnitDecloaked(const CUnit* unit) {}

		virtual void RenderUnitCreated(const CUnit* unit, int cloaked) {}
		virtual void RenderUnitDestroyed(const CUnit* unit) {}

		virtual bool UnitUnitCollision(const CUnit* collider, const CUnit* collidee) { return false; }
		virtual bool UnitFeatureCollision(const CUnit* collider, const CFeature* collidee) { return false; }
		virtual void UnitMoved(const CUnit* unit) {}
		virtual void UnitMoveFailed(const CUnit* unit) {}

		virtual void FeatureCreated(const CFeature* feature) {}
		virtual void FeatureDestroyed(const CFeature* feature) {}
		virtual void FeatureDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID) {}
		virtual void FeatureMoved(const CFeature* feature, const float3& oldpos) {}

		virtual void RenderFeatureCreated(const CFeature* feature) {}
		virtual void RenderFeatureDestroyed(const CFeature* feature) {}

		virtual void ProjectileCreated(const CProjectile* proj) {}
		virtual void ProjectileDestroyed(const CProjectile* proj) {}

		virtual void RenderProjectileCreated(const CProjectile* proj) {}
		virtual void RenderProjectileDestroyed(const CProjectile* proj) {}

		virtual void StockpileChanged(const CUnit* unit,
		                              const CWeapon* weapon, int oldCount) {}

		virtual bool Explosion(int weaponID, int projectileID, const float3& pos, const CUnit* owner) { return false; }


		virtual bool CommandFallback(const CUnit* unit, const Command& cmd) { return false; }
		virtual bool AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced) { return true; }

		virtual bool AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo) { return true; }
		virtual bool AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture) { return true; }
		virtual bool AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part) { return true; }
		virtual bool AllowUnitTransport(const CUnit* transporter, const CUnit* transportee) { return true; }
		virtual bool AllowUnitTransportLoad(const CUnit* transporter, const CUnit* transportee, const float3& loadPos, bool allowed) { return true; }
		virtual bool AllowUnitTransportUnload(const CUnit* transporter, const CUnit* transportee, const float3& unloadPos, bool allowed) { return true; }
		virtual bool AllowUnitCloak(const CUnit* unit, const CUnit* enemy) { return true; }
		virtual bool AllowUnitDecloak(const CUnit* unit, const CSolidObject* object, const CWeapon* weapon) { return true; }
		virtual bool AllowUnitKamikaze(const CUnit* unit, const CUnit* target, bool allowed) { return true; }
		virtual bool AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos) { return true; }
		virtual bool AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part) { return true; }
		virtual bool AllowResourceLevel(int teamID, const string& type, float level) { return true; }
		virtual bool AllowResourceTransfer(int oldTeam, int newTeam, const char* type, float amount) { return true; }
		virtual bool AllowDirectUnitControl(int playerID, const CUnit* unit) { return true; }
		virtual bool AllowBuilderHoldFire(const CUnit* unit, int action) { return true; }
		virtual bool AllowStartPosition(int playerID, int teamID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos) { return true; }

		virtual bool TerraformComplete(const CUnit* unit, const CUnit* build) { return false; }
		virtual bool MoveCtrlNotify(const CUnit* unit, int data) { return false; }

		virtual int AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID) { return -1; }
		virtual bool AllowWeaponTarget(
			unsigned int attackerID,
			unsigned int targetID,
			unsigned int attackerWeaponNum,
			unsigned int attackerWeaponDefID,
			float* targetPriority
		) { return true; }
		virtual bool AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget) { return true; }

		virtual bool UnitPreDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer,
			float* newDamage,
			float* impulseMult
		) { return false; }

		virtual bool FeaturePreDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			float* newDamage,
			float* impulseMult
		) { return false; }

		virtual bool ShieldPreDamaged(
			const CProjectile* projectile,
			const CWeapon* shieldEmitter,
			const CUnit* shieldCarrier,
			bool bounceProjectile,
			const CWeapon* beamEmitter,
			const CUnit* beamCarrier,
			const float3& startPos,
			const float3& hitPos
		) { return false; }

		virtual bool SyncedActionFallback(const string& line, int playerID) { return false; }
		/// @}

		/**
		 * @name Unsynced_events
		 * @{
		 */
		virtual void Save(zipFile archive);

		virtual void Update();
		virtual void UnsyncedHeightMapUpdate(const SRectangle& rect);

		virtual bool KeyPress(int key, bool isRepeat);
		virtual bool KeyRelease(int key);
		virtual bool TextInput(const std::string& utf8);
		virtual bool TextEditing(const std::string& utf8, unsigned int start, unsigned int length);
		virtual bool MouseMove(int x, int y, int dx, int dy, int button);
		virtual bool MousePress(int x, int y, int button);
		virtual void MouseRelease(int x, int y, int button);
		virtual bool MouseWheel(bool up, float value);

		virtual void DownloadQueued(int ID, const string& archiveName, const string& archiveType);
		virtual void DownloadStarted(int ID);
		virtual void DownloadFinished(int ID);
		virtual void DownloadFailed(int ID, int errorID);
		virtual void DownloadProgress(int ID, long downloaded, long total);

		virtual bool IsAbove(int x, int y);
		virtual std::string GetTooltip(int x, int y);

		virtual bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		virtual bool CommandNotify(const Command& cmd);

		virtual bool AddConsoleLine(const std::string& msg, const std::string& section, int level);

		virtual void LastMessagePosition(const float3& pos);

		virtual bool GroupChanged(int groupID);

		virtual bool GameSetup(const std::string& state, bool& ready,
		                       const std::vector< std::pair<int, std::string> >& playerStates);

		virtual std::string WorldTooltip(const CUnit* unit,
		                                 const CFeature* feature,
		                                 const float3* groundPos);

		virtual bool MapDrawCmd(int playerID, int type,
		                        const float3* pos0,
		                        const float3* pos1,
		                        const std::string* label);

		virtual void SunChanged();

		virtual void ViewResize();

		virtual void DrawGenesis() {}
		virtual void DrawWater() {}
		virtual void DrawSky() {}
		virtual void DrawSun() {}
		virtual void DrawGrass() {}
		virtual void DrawTrees() {}
		virtual void DrawWorld() {}
		virtual void DrawWorldPreUnit() {}
		virtual void DrawWorldPreParticles() {}
		virtual void DrawWorldShadow() {}
		virtual void DrawWorldReflection() {}
		virtual void DrawWorldRefraction() {}
		virtual void DrawGroundPreForward() {}
		virtual void DrawGroundPostForward() {}
		virtual void DrawGroundPreDeferred() {}
		virtual void DrawGroundPostDeferred() {}
		virtual void DrawUnitsPostDeferred() {}
		virtual void DrawFeaturesPostDeferred() {}
		virtual void DrawScreenEffects() {}
		virtual void DrawScreenPost() {}
		virtual void DrawScreen() {}
		virtual void DrawInMiniMap() {}
		virtual void DrawInMiniMapBackground() {}

		virtual bool DrawUnit(const CUnit* unit) { return false; }
		virtual bool DrawFeature(const CFeature* feature) { return false; }
		virtual bool DrawShield(const CUnit* unit, const CWeapon* weapon) { return false; }
		virtual bool DrawProjectile(const CProjectile* projectile) { return false; }
		virtual bool DrawMaterial(const LuaMaterial* material) { return false; }

		virtual void GameProgress(int gameFrame);

		virtual void DrawLoadScreen();
		virtual void LoadProgress(const std::string& msg, const bool replace_lastline);

		virtual void CollectGarbage() {}
		virtual void DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end) {}
		virtual void Pong(uint8_t pingTag, const spring_time pktSendTime, const spring_time pktRecvTime) {}
		virtual void MetalMapChanged(const int x, const int z) {}
		/// @}
};


#endif /* EVENT_CLIENT_H */
