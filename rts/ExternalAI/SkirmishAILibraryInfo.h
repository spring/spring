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
*/

#ifndef _SKIRMISHAILIBRARYINFO_H
#define _SKIRMISHAILIBRARYINFO_H

#include <vector>
#include <map>
#include <string>

class ISkirmishAILibrary;
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
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual const std::string& GetShortName() const;
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual const std::string& GetVersion() const;
	virtual const std::string& GetName() const;
	virtual const std::string& GetDescription() const;
	virtual const std::string& GetURL() const;
	virtual const std::string& GetInterfaceShortName() const;
	virtual const std::string& GetInterfaceVersion() const;
	virtual const std::string& GetInfo(const std::string& key) const;

	virtual void SetDataDir(const std::string& dataDir);
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual void SetShortName(const std::string& shortName);
	/** Restrictions: none of the following: spaces, '_', '#' */
	virtual void SetVersion(const std::string& version);
	virtual void SetName(const std::string& name);
	virtual void SetDescription(const std::string& description);
	virtual void SetURL(const std::string& url);
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
