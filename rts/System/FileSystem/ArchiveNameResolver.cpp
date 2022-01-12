/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ArchiveNameResolver.h"

#include "Game/GlobalUnsynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/UriParser.h"
#include "System/StringUtil.h"

#include <iostream>
#include <cinttypes>

namespace ArchiveNameResolver {
	//////////////////////////////////////////////////////////////////////////////
//
//  Helpers
//

	static std::uint64_t ExtractVersionNumber(const std::string& version)
	{
		std::istringstream iss(version);
		std::uint64_t versionInt = 0;
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
			if (aData.IsGame()) {
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
		std::uint64_t matchingVersionInt = 0;

		for (const auto& archiveData: found) {
			if (lowerLazyName != StringToLower(archiveData.GetShortName()))
				continue;

			// find latest version of the game
			std::uint64_t versionInt = ExtractVersionNumber(archiveData.GetVersion());

			if (versionInt > matchingVersionInt) {
				matchingName = archiveData.GetNameVersioned();
				matchingVersion = archiveData.GetVersion();
				matchingVersionInt = versionInt;
				continue;
			}

			if (versionInt == matchingVersionInt) {
				// very bad solution, fails with `10.0` vs. `9.10`
				const int compareInt = matchingVersion.compare(archiveData.GetVersion());
				if (compareInt <= 0) {
					matchingName = archiveData.GetNameVersioned();
					matchingVersion = archiveData.GetVersion();
					//matchingVersionInt = versionInt;
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
#if !defined(UNITSYNC) && !defined(DEDICATED)
		if (lazyName == "random") {
			const std::vector<CArchiveScanner::ArchiveData>& games = archiveScanner->GetPrimaryMods();
			if (!games.empty()) {
				*applicableName = games[guRNG.NextInt(games.size())].GetNameVersioned();
				return true;
			}
		}
#endif //UNITSYNC
		return false;
	}


	static bool GetMapByExactName(const std::string& lazyName, std::string* applicableName)
	{
		const CArchiveScanner::ArchiveData& aData = archiveScanner->GetArchiveData(lazyName);

		std::string error;
		if (aData.IsValid(error)) {
			if (aData.IsMap()) {
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

		for (const auto& mapName: found) {
			const std::string lowerMapName = StringToLower(mapName);

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
					matchingName = mapName;
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
#if !defined(UNITSYNC) && !defined(DEDICATED)
		if (lazyName == "random") {
			const std::vector<std::string>& maps = archiveScanner->GetMaps();
			if (!maps.empty()) {
				*applicableName = maps[guRNG.NextInt(maps.size())];
				return true;
			}
		}
#endif //UNITSYNC
		return false;
	}

	static bool GetGameByRapidTag(const std::string& lazyName, std::string& tag)
	{
		if (!ParseRapidUri(lazyName, tag))
			return false;

		// NB:
		//   this returns tag as-is (e.g. "zk:stable", unversioned) if no matching rapid entry exists
		//   non-rapid archives should never have names that match rapid tags, otherwise dependencies
		//   like "rapid://zk:stable" would be ambiguous
		tag = GetRapidPackageFromTag(tag);
		return !tag.empty();
	}

//////////////////////////////////////////////////////////////////////////////
//
//  Interface
//

std::string GetGame(const std::string& lazyName)
{
	std::string applicableName = lazyName;
	if (GetGameByExactName(lazyName, &applicableName)) return applicableName;
	if (GetGameByShortName(lazyName, &applicableName)) return applicableName;
	if (GetGameByRapidTag(lazyName, applicableName))   return applicableName;
	if (GetRandomGame(lazyName, &applicableName))      return applicableName;
	if (lazyName == "last")                            return configHandler->GetString("LastSelectedMod");

	return lazyName;
}

std::string GetMap(const std::string& lazyName)
{
	std::string applicableName = lazyName;
	if (GetMapByExactName(lazyName, &applicableName)) return applicableName;
	if (GetMapBySubString(lazyName, &applicableName)) return applicableName;
	if (GetRandomMap(lazyName, &applicableName))      return applicableName;
	if (lazyName == "last")	                          return configHandler->GetString("LastSelectedMap");
	//TODO add a string similarity search?

	return lazyName;
}

} //namespace ArchiveNameResolver
