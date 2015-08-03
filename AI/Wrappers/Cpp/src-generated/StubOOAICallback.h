/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBOOAICALLBACK_H
#define _CPPWRAPPER_STUBOOAICALLBACK_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "OOAICallback.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubOOAICallback : public OOAICallback {

protected:
	virtual ~StubOOAICallback();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	std::vector<springai::Resource*> resources;/* = std::vector<springai::Resource*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetResources(std::vector<springai::Resource*> resources);
	// @Override
	virtual std::vector<springai::Resource*> GetResources();
	// @Override
	virtual springai::Resource* GetResourceByName(const char* resourceName);
	/**
	 * A UnitDef contains all properties of a unit that are specific to its type,
	 * for example the number and type of weapons or max-speed.
	 * These properties are usually fixed, and not meant to change during a game.
	 * The unitId is a unique id for this type of unit.
	 */
private:
	std::vector<springai::UnitDef*> unitDefs;/* = std::vector<springai::UnitDef*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitDefs(std::vector<springai::UnitDef*> unitDefs);
	// @Override
	virtual std::vector<springai::UnitDef*> GetUnitDefs();
	// @Override
	virtual springai::UnitDef* GetUnitDefByName(const char* unitName);
	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS.
	 * If cheats are enabled, this will return all enemies on the map.
	 */
private:
	std::vector<springai::Unit*> enemyUnits;/* = std::vector<springai::Unit*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEnemyUnits(std::vector<springai::Unit*> enemyUnits);
	// @Override
	virtual std::vector<springai::Unit*> GetEnemyUnits();
	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS plus they have to be located in the specified area
	 * of the map.
	 * If cheats are enabled, this will return all enemies
	 * in the specified area.
	 */
	// @Override
	virtual std::vector<springai::Unit*> GetEnemyUnitsIn(const springai::AIFloat3& pos, float radius);
	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in under sensor coverage (sight or radar).
	 * If cheats are enabled, this will return all enemies on the map.
	 */
private:
	std::vector<springai::Unit*> enemyUnitsInRadarAndLos;/* = std::vector<springai::Unit*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEnemyUnitsInRadarAndLos(std::vector<springai::Unit*> enemyUnitsInRadarAndLos);
	// @Override
	virtual std::vector<springai::Unit*> GetEnemyUnitsInRadarAndLos();
	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units.
	 */
private:
	std::vector<springai::Unit*> friendlyUnits;/* = std::vector<springai::Unit*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFriendlyUnits(std::vector<springai::Unit*> friendlyUnits);
	// @Override
	virtual std::vector<springai::Unit*> GetFriendlyUnits();
	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units plus they have to be located in the specified area of the map.
	 */
	// @Override
	virtual std::vector<springai::Unit*> GetFriendlyUnitsIn(const springai::AIFloat3& pos, float radius);
	/**
	 * Returns all units that are neutral and are in LOS.
	 */
private:
	std::vector<springai::Unit*> neutralUnits;/* = std::vector<springai::Unit*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNeutralUnits(std::vector<springai::Unit*> neutralUnits);
	// @Override
	virtual std::vector<springai::Unit*> GetNeutralUnits();
	/**
	 * Returns all units that are neutral and are in LOS plus they have to be
	 * located in the specified area of the map.
	 */
	// @Override
	virtual std::vector<springai::Unit*> GetNeutralUnitsIn(const springai::AIFloat3& pos, float radius);
	/**
	 * Returns all units that are of the team controlled by this AI instance. This
	 * list can also be created dynamically by the AI, through updating a list on
	 * each unit-created and unit-destroyed event.
	 */
private:
	std::vector<springai::Unit*> teamUnits;/* = std::vector<springai::Unit*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeamUnits(std::vector<springai::Unit*> teamUnits);
	// @Override
	virtual std::vector<springai::Unit*> GetTeamUnits();
	/**
	 * Returns all units that are currently selected
	 * (usually only contains units if a human player
	 * is controlling this team as well).
	 */
private:
	std::vector<springai::Unit*> selectedUnits;/* = std::vector<springai::Unit*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSelectedUnits(std::vector<springai::Unit*> selectedUnits);
	// @Override
	virtual std::vector<springai::Unit*> GetSelectedUnits();
private:
	std::vector<springai::Team*> enemyTeams;/* = std::vector<springai::Team*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEnemyTeams(std::vector<springai::Team*> enemyTeams);
	// @Override
	virtual std::vector<springai::Team*> GetEnemyTeams();
private:
	std::vector<springai::Team*> allyTeams;/* = std::vector<springai::Team*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAllyTeams(std::vector<springai::Team*> allyTeams);
	// @Override
	virtual std::vector<springai::Team*> GetAllyTeams();
private:
	std::vector<springai::Group*> groups;/* = std::vector<springai::Group*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGroups(std::vector<springai::Group*> groups);
	// @Override
	virtual std::vector<springai::Group*> GetGroups();
private:
	std::vector<springai::FeatureDef*> featureDefs;/* = std::vector<springai::FeatureDef*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFeatureDefs(std::vector<springai::FeatureDef*> featureDefs);
	// @Override
	virtual std::vector<springai::FeatureDef*> GetFeatureDefs();
	/**
	 * Returns all features currently in LOS, or all features on the map
	 * if cheating is enabled.
	 */
private:
	std::vector<springai::Feature*> features;/* = std::vector<springai::Feature*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFeatures(std::vector<springai::Feature*> features);
	// @Override
	virtual std::vector<springai::Feature*> GetFeatures();
	/**
	 * Returns all features in a specified area that are currently in LOS,
	 * or all features in this area if cheating is enabled.
	 */
	// @Override
	virtual std::vector<springai::Feature*> GetFeaturesIn(const springai::AIFloat3& pos, float radius);
private:
	std::vector<springai::WeaponDef*> weaponDefs;/* = std::vector<springai::WeaponDef*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWeaponDefs(std::vector<springai::WeaponDef*> weaponDefs);
	// @Override
	virtual std::vector<springai::WeaponDef*> GetWeaponDefs();
	// @Override
	virtual springai::WeaponDef* GetWeaponDefByName(const char* weaponDefName);
private:
	springai::SkirmishAI* skirmishAI;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAI(springai::SkirmishAI* skirmishAI);
	// @Override
	virtual springai::SkirmishAI* GetSkirmishAI();
private:
	springai::Pathing* pathing;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPathing(springai::Pathing* pathing);
	// @Override
	virtual springai::Pathing* GetPathing();
private:
	springai::Economy* economy;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEconomy(springai::Economy* economy);
	// @Override
	virtual springai::Economy* GetEconomy();
private:
	springai::Teams* teams;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeams(springai::Teams* teams);
	// @Override
	virtual springai::Teams* GetTeams();
private:
	springai::DataDirs* dataDirs;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDataDirs(springai::DataDirs* dataDirs);
	// @Override
	virtual springai::DataDirs* GetDataDirs();
private:
	springai::Game* game;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGame(springai::Game* game);
	// @Override
	virtual springai::Game* GetGame();
private:
	springai::Cheats* cheats;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCheats(springai::Cheats* cheats);
	// @Override
	virtual springai::Cheats* GetCheats();
private:
	springai::Map* map;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMap(springai::Map* map);
	// @Override
	virtual springai::Map* GetMap();
private:
	springai::Log* log;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLog(springai::Log* log);
	// @Override
	virtual springai::Log* GetLog();
private:
	springai::Debug* debug;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDebug(springai::Debug* debug);
	// @Override
	virtual springai::Debug* GetDebug();
private:
	springai::File* file;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFile(springai::File* file);
	// @Override
	virtual springai::File* GetFile();
private:
	springai::Gui* gui;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGui(springai::Gui* gui);
	// @Override
	virtual springai::Gui* GetGui();
private:
	springai::Lua* lua;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLua(springai::Lua* lua);
	// @Override
	virtual springai::Lua* GetLua();
private:
	springai::Mod* mod;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMod(springai::Mod* mod);
	// @Override
	virtual springai::Mod* GetMod();
private:
	springai::Engine* engine;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEngine(springai::Engine* engine);
	// @Override
	virtual springai::Engine* GetEngine();
private:
	springai::SkirmishAIs* skirmishAIs;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIs(springai::SkirmishAIs* skirmishAIs);
	// @Override
	virtual springai::SkirmishAIs* GetSkirmishAIs();
}; // class StubOOAICallback

}  // namespace springai

#endif // _CPPWRAPPER_STUBOOAICALLBACK_H

