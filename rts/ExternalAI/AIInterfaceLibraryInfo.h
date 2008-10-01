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

#ifndef _AIINTERFACELIBRARYINFO_H
#define	_AIINTERFACELIBRARYINFO_H

#include "Interface/SAIInterfaceLibrary.h"

#include <map>
#include <string>

class IAIInterfaceLibrary;
struct InfoItem;

class CAIInterfaceLibraryInfo {
public:
	CAIInterfaceLibraryInfo(const IAIInterfaceLibrary& interface);
	CAIInterfaceLibraryInfo(const CAIInterfaceLibraryInfo& interfaceInfo);
	CAIInterfaceLibraryInfo(const std::string& interfaceInfoFile,
			const std::string& fileModes,
			const std::string& accessModes);
    
    virtual LevelOfSupport GetLevelOfSupportForCurrentEngine() const;
    
    virtual std::string GetFileName() const; // when the AI is "libRAI-0.600.so" or "RAI-0.600.dll", this value should be "RAI-0.600"
    virtual std::string GetShortName() const; // restrictions: none of the following: spaces, '_', '#'
    virtual std::string GetVersion() const; // restrictions: none of the following: spaces, '_', '#'
    virtual std::string GetName() const;
    virtual std::string GetDescription() const;
    virtual std::string GetURL() const;
    virtual std::string GetInfo(const std::string& key) const;
    virtual const std::map<std::string, InfoItem>* GetInfos() const;
	
    virtual void SetFileName(const std::string& fileName); // when the AI is "libRAI-0.600.so" or "RAI-0.600.dll", this value should be "RAI-0.600"
    virtual void SetShortName(const std::string& shortName); // restrictions: none of the following: spaces, '_', '#'
    virtual void SetVersion(const std::string& version); // restrictions: none of the following: spaces, '_', '#'
    virtual void SetName(const std::string& name);
    virtual void SetDescription(const std::string& description);
    virtual void SetURL(const std::string& url);
    virtual bool SetInfo(const std::string& key, const std::string& value);
	
private:
	static const unsigned int MAX_INFOS = 128;
	std::map<std::string, InfoItem> infos;
	LevelOfSupport levelOfSupport;
};

#endif	/* _AIINTERFACELIBRARYINFO_H */

