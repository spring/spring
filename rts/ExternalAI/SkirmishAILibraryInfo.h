/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SKIRMISHAILIBRARYINFO_H
#define _SKIRMISHAILIBRARYINFO_H

#include <vector>
#include <map>
#include <string>

class SkirmishAIKey;
struct Option;

class CSkirmishAILibraryInfo {
public:
	CSkirmishAILibraryInfo(const CSkirmishAILibraryInfo& aiInfo);
	CSkirmishAILibraryInfo(const std::string& aiInfoFile,
			const std::string& aiOptionFile = "");
	~CSkirmishAILibraryInfo();

	virtual size_t size() const;
	virtual const std::string& GetKeyAt(size_t index) const;
	virtual const std::string& GetValueAt(size_t index) const;
	virtual const std::string& GetDescriptionAt(size_t index) const;

	virtual SkirmishAIKey GetKey() const;

	virtual const std::string& GetDataDir() const;
	virtual const std::string& GetDataDirCommon() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual const std::string& GetShortName() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual const std::string& GetVersion() const;
	virtual const std::string& GetName() const;
	virtual const std::string& GetDescription() const;
	virtual const std::string& GetURL() const;
	virtual bool IsLuaAI() const;
	virtual const std::string& GetInterfaceShortName() const;
	virtual const std::string& GetInterfaceVersion() const;
	virtual const std::string& GetInfo(const std::string& key) const;

	virtual void SetDataDir(const std::string& dataDir);
	virtual void SetDataDirCommon(const std::string& dataDirCommon);
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual void SetShortName(const std::string& shortName);
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual void SetVersion(const std::string& version);
	virtual void SetName(const std::string& name);
	virtual void SetDescription(const std::string& description);
	virtual void SetURL(const std::string& url);
	virtual void SetLuaAI(const std::string& isLua);
	virtual void SetLuaAI(const bool isLua);
	virtual void SetInterfaceShortName(const std::string& interfaceShortName);
	virtual void SetInterfaceVersion(const std::string& interfaceVersion);
	virtual bool SetInfo(const std::string& key, const std::string& value,
			const std::string& description = "");


	virtual const std::vector<Option>& GetOptions() const;

private:
	// for having a well defined order
	std::vector<std::string> info_keys;
	std::map<std::string, std::string> info_keyLower_key;
	std::map<std::string, std::string> info_key_value;
	std::map<std::string, std::string> info_key_description;

	std::vector<Option> options;
};

#endif // _SKIRMISHAILIBRARYINFO_H
