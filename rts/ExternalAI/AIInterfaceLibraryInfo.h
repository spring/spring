/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_INTERFACE_LIBRARY_INFO_H
#define AI_INTERFACE_LIBRARY_INFO_H

#include <vector>
#include <map>
#include <string>

class AIInterfaceKey;

class CAIInterfaceLibraryInfo {
public:
	CAIInterfaceLibraryInfo() {}
	CAIInterfaceLibraryInfo(const CAIInterfaceLibraryInfo& interfaceInfo);
	CAIInterfaceLibraryInfo(const std::string& interfaceInfoFile);
	~CAIInterfaceLibraryInfo();

	// LevelOfSupport GetLevelOfSupportForCurrentEngine() const;

	size_t size() const;
	const std::string& GetKeyAt(size_t index) const;
	const std::string& GetValueAt(size_t index) const;
	const std::string& GetDescriptionAt(size_t index) const;

	AIInterfaceKey GetKey() const;

	const std::string& GetDataDir() const;
	const std::string& GetDataDirCommon() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	const std::string& GetShortName() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	const std::string& GetVersion() const;
	const std::string& GetName() const;
	const std::string& GetDescription() const;
	const std::string& GetURL() const;
	bool IsLookupSupported() const;
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
	bool SetInfo(const std::string& key, const std::string& value, const std::string& description = "");

private:
	// for having a well defined order
	std::vector<std::string> keys;
	std::map<std::string, std::string> keyLower_key;
	std::map<std::string, std::string> key_value;
	std::map<std::string, std::string> key_description;
};

#endif // AI_INTERFACE_LIBRARY_INFO_H
