/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

namespace StartScriptGen {

	/**
	* creates an empty script.txt with game & map, no error checking is done!
	* @param game name of the game
	* @param map name of the map
	*/
	std::string CreateMinimalSetup(const std::string& game, const std::string& map);

	/**
	* creates an empty script.txt with game & map, only few error checking is done!
	* @param game name of the game
	* @param map name of the map
	* @param ai ai to use (Lua/Skirmish), if not found "empty" player is used
	* @param playername name to use ingame
	*/
	std::string CreateDefaultSetup(const std::string& map, const std::string& game, const std::string& ai, const std::string& playername);
}
