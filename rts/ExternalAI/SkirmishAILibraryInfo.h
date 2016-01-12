/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKIRMISH_AI_LIBRARY_INFO_H
#define SKIRMISH_AI_LIBRARY_INFO_H

#include <map>
#include <vector>
#include <string>

#include "System/Option.h"

class SkirmishAIKey;
struct Option;

class CSkirmishAILibraryInfo {
public:
	CSkirmishAILibraryInfo() {}
	CSkirmishAILibraryInfo(const CSkirmishAILibraryInfo& aiInfo);

	/**
	 * This is used when initializing from purely Lua files
	 * (AIInfo.lua & AIOptions.lua).
	 */
	CSkirmishAILibraryInfo(const std::string& aiInfoFile, const std::string& aiOptionFile = "");
	/**
	 * This is used when initializing from data fetched through the AI Interface
	 * library plugin, through C functions.
	 */
	CSkirmishAILibraryInfo(const std::map<std::string, std::string>& aiInfo, const std::string& aiOptionLua = "");

	size_t size() const { return info_keys.size(); }
	const std::string& GetKeyAt(size_t index) const;
	const std::string& GetValueAt(size_t index) const;
	const std::string& GetDescriptionAt(size_t index) const;

	SkirmishAIKey GetKey() const;

	const std::string& GetDataDir() const;
	const std::string& GetDataDirCommon() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	const std::string& GetShortName() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	const std::string& GetVersion() const;
	const std::string& GetName() const;
	const std::string& GetDescription() const;
	const std::string& GetURL() const;
	bool IsLuaAI() const;
	const std::string& GetInterfaceShortName() const;
	const std::string& GetInterfaceVersion() const;
	const std::string& GetInfo(const std::string& key) const;

	void SetDataDir(const std::string& dataDir);
	void SetDataDirCommon(const std::string& dataDirCommon);
	/** Restrictions: none of the following: spaces, '_', '#' */
	void SetShortName(const std::string& shortName);
	/** Restrictions: none of the following: spaces, '_', '#' */
	void SetVersion(const std::string& version);
	void SetName(const std::string& name);
	void SetDescription(const std::string& description);
	void SetURL(const std::string& url);
	void SetLuaAI(const std::string& isLua);
	void SetLuaAI(const bool isLua);
	void SetInterfaceShortName(const std::string& interfaceShortName);
	void SetInterfaceVersion(const std::string& interfaceVersion);
	bool SetInfo(const std::string& key, const std::string& value, const std::string& description = "");

	const std::vector<Option>& GetOptions() const { return options; }

private:
	// for having a well defined order
	std::vector<std::string> info_keys;
	std::map<std::string, std::string> info_keyLower_key;
	std::map<std::string, std::string> info_key_value;
	std::map<std::string, std::string> info_key_description;

	std::vector<Option> options;
};

#endif // SKIRMISH_AI_LIBRARY_INFO_H
