/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	@author	Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef __SKIRMISH_AI_HANDLER_H
#define __SKIRMISH_AI_HANDLER_H

#include "ExternalAI/SkirmishAIData.h"

#include "creg/creg_cond.h"

#include <map>
#include <set>


class CGameSetup;

/**
 * Handles all Skirmish AI instance relevant data, which includes,
 * but is not limited to all sync relevant Skirmish AI stuff.
 */
class CSkirmishAIHandler
{
private:
	CR_DECLARE(CSkirmishAIHandler);

	CSkirmishAIHandler();
	~CSkirmishAIHandler();

public:
	//typedef std::vector<SkirmishAIData*> ais_t;
	typedef std::vector<size_t> ids_t;

	/**
	 * Fetcher for the singleton.
	 */
	static CSkirmishAIHandler& GetInstance();

	void LoadFromSetup(const CGameSetup& setup);

	/**
	 * @brief SkirmishAIData
	 * @param skirmishAiId index to fetch
	 * @return SkirmishAIData pointer
	 *
	 * Accesses the data of a Skirmish AI instance at a given index.
	 */
	SkirmishAIData* GetSkirmishAI(const size_t skirmishAiId);

	/**
	 * @brief SkirmishAIData
	 * @param name name of the Skirmish AI instance
	 * @return its index or -1 if not found
	 *
	 * Search a Skirmish AI instance by name.
	 */
	size_t GetSkirmishAI(const std::string& name) const;

	/**
	 * @brief Skirmish AIs controlling a team
	 * 
	 * Will change during runtime (Connection lost, died, killed, created, ...).
	 */
	ids_t GetSkirmishAIsInTeam(const int teamId, const int playerId = -1);

	/**
	 * @brief Skirmish AIs hosted by a player
	 * 
	 * Will change during runtime (Connection lost, died, killed, created, ...).
	 */
	ids_t GetSkirmishAIsByPlayer(const int playerId);

	/**
	 * @brief All active Skirmish AIs
	 * 
	 * Will change during runtime (Connection lost, died, killed, created, ...).
	 */
	ids_t GetAllSkirmishAIs();

	/**
	 * @brief Adds a Skirmish AI
	 * @param data the Skirmish AI to add
	 * @return the id of the Skirmish AI that was added
	 */
	size_t AddSkirmishAI(const SkirmishAIData& data);

	/**
	 * @brief Removes a Skirmish AI
	 * @param data the Skirmish AI to add
	 * @return true if an Skirmish AI was removed
	 */
	bool RemoveSkirmishAI(const size_t skirmishAiId);

	/**
	 * Called when a Skirmish AI left the scene.
	 *
	 * @param skirmishAiId Skirmish AI index
	 * @param reason see ...?
	 */
	//void SkirmishAILeft(int skirmishAiId, unsigned char reason);

	/**
	 * @brief Number of players the game was created for
	 * 
	 * Constant at runtime
	 */
	size_t GetNumSkirmishAIs() const;
	

	//const std::vector<std::string>& GetSkirmishAIOptionValueKeys(int teamId) const;
	//const std::map<std::string, std::string>& GetSkirmishAIOptionValues(int teamId) const;

	//const T_skirmishAIInfos& GetUsedSkirmishAIInfos();

private:
	/// The Id reserved of for the next AI that will be added.
	size_t nextId;

	typedef std::map<size_t, SkirmishAIData> id_ai_t;
	/// Id -> AI instance
	id_ai_t id_ai;

	/*std::map<int, std::vector<std::string> > teamId_skirmishAIOptionValueKeys;
	static const std::vector<std::string> EMPTY_OPTION_VALUE_KEYS;
	std::map<int, std::map<std::string, std::string> > teamId_skirmishAIOptionValues;
	static const std::map<std::string, std::string> EMPTY_OPTION_VALUES;

	T_skirmishAIInfos usedSkirmishAIInfos;
	bool usedSkirmishAIInfos_initialized;*/

	static CSkirmishAIHandler* mySingleton;
};

#define skirmishAIHandler CSkirmishAIHandler::GetInstance()

#endif // __SKIRMISH_AI_HANDLER_H
