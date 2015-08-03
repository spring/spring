/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_OOAICALLBACK_H
#define _CPPWRAPPER_OOAICALLBACK_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class OOAICallback {

public:
	virtual ~OOAICallback(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual std::vector<springai::Resource*> GetResources() = 0;

public:
	virtual springai::Resource* GetResourceByName(const char* resourceName) = 0;

	/**
	 * A UnitDef contains all properties of a unit that are specific to its type,
	 * for example the number and type of weapons or max-speed.
	 * These properties are usually fixed, and not meant to change during a game.
	 * The unitId is a unique id for this type of unit.
	 */
public:
	virtual std::vector<springai::UnitDef*> GetUnitDefs() = 0;

public:
	virtual springai::UnitDef* GetUnitDefByName(const char* unitName) = 0;

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS.
	 * If cheats are enabled, this will return all enemies on the map.
	 */
public:
	virtual std::vector<springai::Unit*> GetEnemyUnits() = 0;

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS plus they have to be located in the specified area
	 * of the map.
	 * If cheats are enabled, this will return all enemies
	 * in the specified area.
	 */
public:
	virtual std::vector<springai::Unit*> GetEnemyUnitsIn(const springai::AIFloat3& pos, float radius) = 0;

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in under sensor coverage (sight or radar).
	 * If cheats are enabled, this will return all enemies on the map.
	 */
public:
	virtual std::vector<springai::Unit*> GetEnemyUnitsInRadarAndLos() = 0;

	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units.
	 */
public:
	virtual std::vector<springai::Unit*> GetFriendlyUnits() = 0;

	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units plus they have to be located in the specified area of the map.
	 */
public:
	virtual std::vector<springai::Unit*> GetFriendlyUnitsIn(const springai::AIFloat3& pos, float radius) = 0;

	/**
	 * Returns all units that are neutral and are in LOS.
	 */
public:
	virtual std::vector<springai::Unit*> GetNeutralUnits() = 0;

	/**
	 * Returns all units that are neutral and are in LOS plus they have to be
	 * located in the specified area of the map.
	 */
public:
	virtual std::vector<springai::Unit*> GetNeutralUnitsIn(const springai::AIFloat3& pos, float radius) = 0;

	/**
	 * Returns all units that are of the team controlled by this AI instance. This
	 * list can also be created dynamically by the AI, through updating a list on
	 * each unit-created and unit-destroyed event.
	 */
public:
	virtual std::vector<springai::Unit*> GetTeamUnits() = 0;

	/**
	 * Returns all units that are currently selected
	 * (usually only contains units if a human player
	 * is controlling this team as well).
	 */
public:
	virtual std::vector<springai::Unit*> GetSelectedUnits() = 0;

public:
	virtual std::vector<springai::Team*> GetEnemyTeams() = 0;

public:
	virtual std::vector<springai::Team*> GetAllyTeams() = 0;

public:
	virtual std::vector<springai::Group*> GetGroups() = 0;

public:
	virtual std::vector<springai::FeatureDef*> GetFeatureDefs() = 0;

	/**
	 * Returns all features currently in LOS, or all features on the map
	 * if cheating is enabled.
	 */
public:
	virtual std::vector<springai::Feature*> GetFeatures() = 0;

	/**
	 * Returns all features in a specified area that are currently in LOS,
	 * or all features in this area if cheating is enabled.
	 */
public:
	virtual std::vector<springai::Feature*> GetFeaturesIn(const springai::AIFloat3& pos, float radius) = 0;

public:
	virtual std::vector<springai::WeaponDef*> GetWeaponDefs() = 0;

public:
	virtual springai::WeaponDef* GetWeaponDefByName(const char* weaponDefName) = 0;

public:
	virtual springai::SkirmishAI* GetSkirmishAI() = 0;

public:
	virtual springai::Pathing* GetPathing() = 0;

public:
	virtual springai::Economy* GetEconomy() = 0;

public:
	virtual springai::Teams* GetTeams() = 0;

public:
	virtual springai::DataDirs* GetDataDirs() = 0;

public:
	virtual springai::Game* GetGame() = 0;

public:
	virtual springai::Cheats* GetCheats() = 0;

public:
	virtual springai::Map* GetMap() = 0;

public:
	virtual springai::Log* GetLog() = 0;

public:
	virtual springai::Debug* GetDebug() = 0;

public:
	virtual springai::File* GetFile() = 0;

public:
	virtual springai::Gui* GetGui() = 0;

public:
	virtual springai::Lua* GetLua() = 0;

public:
	virtual springai::Mod* GetMod() = 0;

public:
	virtual springai::Engine* GetEngine() = 0;

public:
	virtual springai::SkirmishAIs* GetSkirmishAIs() = 0;

}; // class OOAICallback

}  // namespace springai

#endif // _CPPWRAPPER_OOAICALLBACK_H

