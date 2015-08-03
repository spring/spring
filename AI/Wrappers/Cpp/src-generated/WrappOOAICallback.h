/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPOOAICALLBACK_H
#define _CPPWRAPPER_WRAPPOOAICALLBACK_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "OOAICallback.h"

struct SSkirmishAICallback;

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappOOAICallback : public OOAICallback {

private:
	const struct SSkirmishAICallback* innerCallback;
	int skirmishAIId;

	WrappOOAICallback(const struct SSkirmishAICallback* innerCallback, int skirmishAIId);
	virtual ~WrappOOAICallback();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static OOAICallback* GetInstance(const struct SSkirmishAICallback* innerCallback, int skirmishAIId);

public:
	// @Override
	virtual std::vector<springai::Resource*> GetResources();

public:
	// @Override
	virtual springai::Resource* GetResourceByName(const char* resourceName);

	/**
	 * A UnitDef contains all properties of a unit that are specific to its type,
	 * for example the number and type of weapons or max-speed.
	 * These properties are usually fixed, and not meant to change during a game.
	 * The unitId is a unique id for this type of unit.
	 */
public:
	// @Override
	virtual std::vector<springai::UnitDef*> GetUnitDefs();

public:
	// @Override
	virtual springai::UnitDef* GetUnitDefByName(const char* unitName);

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS.
	 * If cheats are enabled, this will return all enemies on the map.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetEnemyUnits();

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in LOS plus they have to be located in the specified area
	 * of the map.
	 * If cheats are enabled, this will return all enemies
	 * in the specified area.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetEnemyUnitsIn(const springai::AIFloat3& pos, float radius);

	/**
	 * Returns all units that are not in this teams ally-team nor neutral
	 * and are in under sensor coverage (sight or radar).
	 * If cheats are enabled, this will return all enemies on the map.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetEnemyUnitsInRadarAndLos();

	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetFriendlyUnits();

	/**
	 * Returns all units that are in this teams ally-team, including this teams
	 * units plus they have to be located in the specified area of the map.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetFriendlyUnitsIn(const springai::AIFloat3& pos, float radius);

	/**
	 * Returns all units that are neutral and are in LOS.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetNeutralUnits();

	/**
	 * Returns all units that are neutral and are in LOS plus they have to be
	 * located in the specified area of the map.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetNeutralUnitsIn(const springai::AIFloat3& pos, float radius);

	/**
	 * Returns all units that are of the team controlled by this AI instance. This
	 * list can also be created dynamically by the AI, through updating a list on
	 * each unit-created and unit-destroyed event.
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetTeamUnits();

	/**
	 * Returns all units that are currently selected
	 * (usually only contains units if a human player
	 * is controlling this team as well).
	 */
public:
	// @Override
	virtual std::vector<springai::Unit*> GetSelectedUnits();

public:
	// @Override
	virtual std::vector<springai::Team*> GetEnemyTeams();

public:
	// @Override
	virtual std::vector<springai::Team*> GetAllyTeams();

public:
	// @Override
	virtual std::vector<springai::Group*> GetGroups();

public:
	// @Override
	virtual std::vector<springai::FeatureDef*> GetFeatureDefs();

	/**
	 * Returns all features currently in LOS, or all features on the map
	 * if cheating is enabled.
	 */
public:
	// @Override
	virtual std::vector<springai::Feature*> GetFeatures();

	/**
	 * Returns all features in a specified area that are currently in LOS,
	 * or all features in this area if cheating is enabled.
	 */
public:
	// @Override
	virtual std::vector<springai::Feature*> GetFeaturesIn(const springai::AIFloat3& pos, float radius);

public:
	// @Override
	virtual std::vector<springai::WeaponDef*> GetWeaponDefs();

public:
	// @Override
	virtual springai::WeaponDef* GetWeaponDefByName(const char* weaponDefName);

public:
	// @Override
	virtual springai::SkirmishAI* GetSkirmishAI();

public:
	// @Override
	virtual springai::Pathing* GetPathing();

public:
	// @Override
	virtual springai::Economy* GetEconomy();

public:
	// @Override
	virtual springai::Teams* GetTeams();

public:
	// @Override
	virtual springai::DataDirs* GetDataDirs();

public:
	// @Override
	virtual springai::Game* GetGame();

public:
	// @Override
	virtual springai::Cheats* GetCheats();

public:
	// @Override
	virtual springai::Map* GetMap();

public:
	// @Override
	virtual springai::Log* GetLog();

public:
	// @Override
	virtual springai::Debug* GetDebug();

public:
	// @Override
	virtual springai::File* GetFile();

public:
	// @Override
	virtual springai::Gui* GetGui();

public:
	// @Override
	virtual springai::Lua* GetLua();

public:
	// @Override
	virtual springai::Mod* GetMod();

public:
	// @Override
	virtual springai::Engine* GetEngine();

public:
	// @Override
	virtual springai::SkirmishAIs* GetSkirmishAIs();
}; // class WrappOOAICallback

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPOOAICALLBACK_H

