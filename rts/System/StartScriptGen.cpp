/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <iostream>
#include "StartScriptGen.h"
#include "AIScriptHandler.h"
#include "Game/GlobalUnsynced.h"
#include "System/TdfParser.h"
#include "System/Util.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/UriParser.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/Log/ILog.h"
#include <boost/cstdint.hpp>


CONFIG(bool, NoHelperAIs).defaultValue(false);

namespace StartScriptGen {

//////////////////////////////////////////////////////////////////////////////
//
//  Helpers
//

	static boost::uint64_t ExtractVersionNumber(const std::string& version)
	{
		std::istringstream iss(version);
		boost::uint64_t versionInt = 0;
		int num;
		while (true) {
			if (iss >> num) {
				versionInt = versionInt * 1000 + std::abs(num);
			} else
			if (iss.eof()) {
				break;
			} else
			if (iss.fail()) {
				iss.clear();
				iss.ignore();
			}
		}
		return versionInt;
	}


	static bool GetGameByExactName(const std::string& lazyName, std::string* applicableName)
	{
		const CArchiveScanner::ArchiveData& aData = archiveScanner->GetArchiveData(lazyName);

		std::string error;
		if (aData.IsValid(error)) {
			if (aData.GetModType() == modtype::primary) {
				*applicableName = lazyName;
				return true;
			}
		}

		return false;
	}


	static bool GetGameByShortName(const std::string& lazyName, std::string* applicableName)
	{
		const std::string lowerLazyName = StringToLower(lazyName);
		const std::vector<CArchiveScanner::ArchiveData>& found = archiveScanner->GetPrimaryMods();

		std::string matchingName;
		std::string matchingVersion;
		boost::uint64_t matchingVersionInt = 0;

		for (std::vector<CArchiveScanner::ArchiveData>::const_iterator it = found.begin(); it != found.end(); ++it) {
			if (lowerLazyName == StringToLower(it->GetShortName())) {
				// find latest version of the game
				boost::uint64_t versionInt = ExtractVersionNumber(it->GetVersion());

				if (versionInt > matchingVersionInt) {
					matchingName = it->GetNameVersioned();
					matchingVersion = it->GetVersion();
					matchingVersionInt = versionInt;
					continue;
				}

				if (versionInt == matchingVersionInt) {
					// very bad solution, fails with `10.0` vs. `9.10`
					const int compareInt = matchingVersion.compare(it->GetVersion());
					if (compareInt <= 0) {
						matchingName = it->GetNameVersioned();
						matchingVersion = it->GetVersion();
						//matchingVersionInt = versionInt;
					}
				}
			}
		}

		if (!matchingName.empty()) {
			*applicableName = matchingName;
			return true;
		}

		return false;
	}


	static bool GetRandomGame(const std::string& lazyName, std::string* applicableName)
	{
		if (std::string("random").find(lazyName) != std::string::npos) {
			const std::vector<CArchiveScanner::ArchiveData>& games = archiveScanner->GetPrimaryMods();
			if (!games.empty()) {
				*applicableName = games[gu->RandInt() % games.size()].GetNameVersioned();
				return true;
			}
		}
		return false;
	}


	static bool GetMapByExactName(const std::string& lazyName, std::string* applicableName)
	{
		const CArchiveScanner::ArchiveData& aData = archiveScanner->GetArchiveData(lazyName);

		std::string error;
		if (aData.IsValid(error)) {
			if (aData.GetModType() == modtype::map) {
				*applicableName = lazyName;
				return true;
			}
		}

		return false;
	}


	static bool GetMapBySubString(const std::string& lazyName, std::string* applicableName)
	{
		const std::string lowerLazyName = StringToLower(lazyName);
		const std::vector<std::string>& found = archiveScanner->GetMaps();

		std::vector<std::string> substrings;
		std::istringstream iss(lowerLazyName);
		std::string buf;
		while (iss >> buf) {
			substrings.push_back(buf);
		}

		std::string matchingName;
		size_t matchingLength = 1e6;

		for (std::vector<std::string>::const_iterator it = found.begin(); it != found.end(); ++it) {
			const std::string lowerMapName = StringToLower(*it);

			// search for all wanted substrings
			bool fits = true;
			for (std::vector<std::string>::const_iterator jt = substrings.begin(); jt != substrings.end(); ++jt) {
				const std::string& substr = *jt;
				if (lowerMapName.find(substr) == std::string::npos) {
					fits = false;
					break;
				}
			}

			if (fits) {
				// shortest fitting string wins
				const int nameLength = lowerMapName.length();
				if (nameLength < matchingLength) {
					matchingName = *it;
					matchingLength = nameLength;
				}
			}
		}

		if (!matchingName.empty()) {
			*applicableName = matchingName;
			return true;
		}

		return false;
	}


	static bool GetRandomMap(const std::string& lazyName, std::string* applicableName)
	{
		if (std::string("random").find(lazyName) != std::string::npos) {
			const std::vector<std::string>& maps = archiveScanner->GetMaps();
			if (!maps.empty()) {
				*applicableName = maps[gu->RandInt() % maps.size()];
				return true;
			}
		}
		return false;
	}

	static bool GetGameByRapidTag(const std::string& lazyName, std::string& tag)
	{
		if (!ParseRapidUri(lazyName, tag))
			return false;
		tag = GetRapidName(tag);
		return !tag.empty();
	}


	static std::string GetGame(const std::string& lazyName)
	{
		std::string applicableName = lazyName;
		if (GetGameByExactName(lazyName, &applicableName)) return applicableName;
		if (GetGameByShortName(lazyName, &applicableName)) return applicableName;
		if (GetRandomGame(lazyName, &applicableName))      return applicableName;
		if (GetGameByRapidTag(lazyName, applicableName))   return applicableName;

		return lazyName;
	}


	static std::string GetMap(const std::string& lazyName)
	{
		std::string applicableName = lazyName;
		if (GetMapByExactName(lazyName, &applicableName)) return applicableName;
		if (GetMapBySubString(lazyName, &applicableName)) return applicableName;
		if (GetRandomMap(lazyName, &applicableName))      return applicableName;
		//TODO add a string similarity search?

		return lazyName;
	}


//////////////////////////////////////////////////////////////////////////////
//
//  Interface
//

std::string CreateMinimalSetup(const std::string& game, const std::string& map)
{
	const std::string playername = configHandler->GetString("name");
	TdfParser::TdfSection setup;
	TdfParser::TdfSection* g = setup.construct_subsection("GAME");

	g->add_name_value("Mapname", GetMap(map));
	g->add_name_value("Gametype", GetGame(game));

	TdfParser::TdfSection* modopts = g->construct_subsection("MODOPTIONS");
	modopts->AddPair("MaxSpeed", 20);
	modopts->AddPair("MinimalSetup", 1); //use for ingame detecting this type of start

	g->AddPair("IsHost", 1);
	g->add_name_value("MyPlayerName", playername);

	TdfParser::TdfSection* player0 = g->construct_subsection("PLAYER0");
	player0->add_name_value("Name", playername);
	player0->AddPair("Team", 0);

	TdfParser::TdfSection* team0 = g->construct_subsection("TEAM0");
	team0->AddPair("TeamLeader", 0);
	team0->AddPair("AllyTeam", 0);

	TdfParser::TdfSection* ally0 = g->construct_subsection("ALLYTEAM0");
	ally0->AddPair("NumAllies", 0);


	std::ostringstream str;
	setup.print(str);
	printf("%s\n", str.str().c_str());
	return str.str();
}


std::string CreateDefaultSetup(const std::string& map, const std::string& game, const std::string& ai,
			const std::string& playername)
{
	//FIXME:: duplicate code with CreateMinimalSetup
	TdfParser::TdfSection setup;
	TdfParser::TdfSection* g = setup.construct_subsection("GAME");
	g->add_name_value("Mapname", map);
	g->add_name_value("Gametype", game);

	TdfParser::TdfSection* modopts = g->construct_subsection("MODOPTIONS");
	modopts->AddPair("MaxSpeed", 20);

	g->AddPair("IsHost", 1);
	g->add_name_value("MyPlayerName", playername);

	g->AddPair("NoHelperAIs", configHandler->GetBool("NoHelperAIs"));

	TdfParser::TdfSection* player0 = g->construct_subsection("PLAYER0");
	player0->add_name_value("Name", playername);
	player0->AddPair("Team", 0);

	const bool isSkirmishAITestScript = CAIScriptHandler::Instance().IsSkirmishAITestScript(ai);
	if (isSkirmishAITestScript) {
		SkirmishAIData aiData = CAIScriptHandler::Instance().GetSkirmishAIData(ai);
		TdfParser::TdfSection* ai = g->construct_subsection("AI0");
		ai->add_name_value("Name", "Enemy");
		ai->add_name_value("ShortName", aiData.shortName);
		ai->add_name_value("Version", aiData.version);
		ai->AddPair("Host", 0);
		ai->AddPair("Team", 1);
	} else if (!ai.empty()) { // is no native ai, try lua ai
		TdfParser::TdfSection* aisec = g->construct_subsection("AI0");
		aisec->add_name_value("Name", "AI: " + ai);
		aisec->add_name_value("ShortName", ai);
		aisec->AddPair("Host", 0);
		aisec->AddPair("Team", 1);
	} else {
		TdfParser::TdfSection* player1 = g->construct_subsection("PLAYER1");
		player1->add_name_value("Name", "Enemy");
		player1->AddPair("Team", 1);
	}

	TdfParser::TdfSection* team0 = g->construct_subsection("TEAM0");
	team0->AddPair("TeamLeader", 0);
	team0->AddPair("AllyTeam", 0);

	TdfParser::TdfSection* team1 = g->construct_subsection("TEAM1");
	if (isSkirmishAITestScript || !ai.empty()) {
		team1->AddPair("TeamLeader", 0);
	} else {
		team1->AddPair("TeamLeader", 1);
	}
	team1->AddPair("AllyTeam", 1);

	TdfParser::TdfSection* ally0 = g->construct_subsection("ALLYTEAM0");
	ally0->AddPair("NumAllies", 0);

	TdfParser::TdfSection* ally1 = g->construct_subsection("ALLYTEAM1");
	ally1->AddPair("NumAllies", 0);

	std::ostringstream str;
	setup.print(str);

	return str.str();
}

}; //namespace StartScriptGen
