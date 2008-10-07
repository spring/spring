/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>
	
	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _GROUPAILIBRARYINFO_H
#define	_GROUPAILIBRARYINFO_H


#include "Interface/ELevelOfSupport.h"

#include <vector>
#include <map>
#include <string>

class IGroupAILibrary;
struct InfoItem;
struct Option;
struct SAIInterfaceSpecifyer;
struct SGAISpecifyer;

class CGroupAILibraryInfo {
public:
	CGroupAILibraryInfo(const IGroupAILibrary& ai, const SAIInterfaceSpecifyer& interfaceSpecifyer);
	CGroupAILibraryInfo(const CGroupAILibraryInfo& aiInfo);
	CGroupAILibraryInfo(
			const std::string& aiInfoFile,
			const std::string& aiOptionFile,
			const std::string& fileModes,
			const std::string& accessModes);
    
    virtual SGAISpecifyer GetSpecifier() const;
	
    virtual std::string GetFileName() const;
    virtual std::string GetShortName() const; // restrictions: none of the following: spaces, '_', '#'
    virtual std::string GetName() const;
    virtual std::string GetVersion() const; // restrictions: none of the following: spaces, '_', '#'
    virtual std::string GetDescription() const;
    virtual std::string GetURL() const;
    virtual std::string GetInterfaceShortName() const;
    virtual std::string GetInterfaceVersion() const;
    virtual std::string GetInfo(const std::string& key) const;
    virtual const std::map<std::string, InfoItem>* GetInfos() const;
	virtual unsigned int GetInfosCReference(InfoItem cInfos[], unsigned int max) const;
	virtual unsigned int GetOptionsCReference(Option cOptions[], unsigned int max) const;
	
	virtual const std::vector<Option>* GetOptions() const;
	
	virtual void SetFileName(const std::string& fileName);
    virtual void SetShortName(const std::string& shortName); // restrictions: none of the following: spaces, '_', '#'
    virtual void SetName(const std::string& name);
    virtual void SetVersion(const std::string& version); // restrictions: none of the following: spaces, '_', '#'
    virtual void SetDescription(const std::string& description);
    virtual void SetURL(const std::string& url);
    virtual void SetInterfaceShortName(const std::string& interfaceShortName);
    virtual void SetInterfaceVersion(const std::string& interfaceVersion);
    virtual bool SetInfo(const std::string& key, const std::string& value);
	
	virtual void SetOptions(const std::vector<Option>& options);
	
private:
	static const unsigned int MAX_INFOS = 128;
	static const unsigned int MAX_OPTIONS = 128;
	std::map<std::string, InfoItem> infos;
	std::vector<Option> options;
};

#endif	/* _GROUPAILIBRARYINFO_H */

