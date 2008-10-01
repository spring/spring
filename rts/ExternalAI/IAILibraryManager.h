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

#ifndef _IAILIBRARYMANAGER_H
#define	_IAILIBRARYMANAGER_H

#include "IAIInterfaceLibrary.h"
#include "ISkirmishAILibrary.h"
//#include "SkirmishAILibraryKey.h"

//#include "AIInterfaceLibraryInfo.h"
struct SSAIKey;
struct SGAIKey;
#include "AIInterfaceLibraryInfo.h"
#include "SkirmishAILibraryInfo.h"
#include "GroupAILibraryInfo.h"

/**
 * @brief manages AIs and AI interfaces
 */
class IAILibraryManager {
//public:
//	typedef s_cont<ISkirmishAILibraryInterfaceInfo>::const_vector T_interfaceInfos_const;
//	typedef s_cont<CSkirmishAILibraryInfoKey>::const_vector T_infoKeys_const;
////	typedef s_cont<ISkirmishAILibraryInfo>::const_vector T_aiInfos_const;
//	typedef s_cont<std::string>::const_vector T_specifyers_const;
	
public:
	virtual ~IAILibraryManager() {}
////	virtual ISkirmishAILibraryManager() = 0; // looks for interface and AIs supported by them (ret != 0: error)
////	virtual ~ISkirmishAILibraryManager() = 0; // unloads all shared libraries that are currently loaded (interfaces and implementations)
//
//	virtual T_interfaceInfos_const GetInterfaceInfos() const = 0;
//	virtual T_infoKeys_const GetInfoKeys() const = 0;
////	virtual const std::vector<const ISkirmishAILibraryInfo*>* GetAIInfos() const = 0;
////	virtual const std::vector<const ISkirmishAILibraryInfo*>* GetInterfaceAIs(const SAIInterfaceLibraryInfo& interfaceInfo) const = 0;
//
//	virtual T_specifyers_const GetSpecifyers() const = 0;
//	virtual T_infoKeys_const GetApplyingInfoKeys(const std::string& specifyer) const {
//		
//		T_infoKeys_const applyingInfoKeys;
//		
//		const T_infoKeys_const infoKeys = GetInfoKeys();
//		T_infoKeys_const::const_iterator infoKey;
//		for (infoKey=infoKeys.begin(); infoKey!=infoKeys.end(); infoKey++) {
//			if ((*infoKey)->IsApplyingSpecifyer(specifyer)) {
//				applyingInfoKeys.push_back(*infoKey);
//			}
//		}
//		
//		return applyingInfoKeys;
//	}
//
//	// an AI is only really loaded when it is not yet loaded.
//	virtual const s_p<const CSkirmishAILibraryKey> LoadAI(const CSkirmishAILibraryInfoKey& infoKey) = 0;
//	// an AI is only unloaded when UnloadAI() is called
//	// as many times as LoadAI() was.
//	// loading and unloading of the interfaces
//	// is handled internally/automatically.
////	virtual int UnloadAI(const CSkirmishAILibraryKey& key) = 0;
//	virtual void UnloadAI(const CSkirmishAILibraryInfoKey& infoKey) = 0;
//	
//	virtual int UnloadEverything() = 0; // unloads all currently loaded AIs and interfaces
//	faces and implementations)
	
	virtual const std::vector<SAIInterfaceSpecifyer>* GetInterfaceSpecifyers() const = 0;
	virtual const std::vector<SSAIKey>* GetSkirmishAIKeys() const = 0;
	virtual const std::vector<SGAIKey>* GetGroupAIKeys() const = 0;
	
//	virtual const std::map<const SAIInterfaceSpecifyer, CAIInterfaceLibraryInfo*>* GetInterfaceInfos() const = 0;
//	virtual const std::map<const SSAIKey, CSkirmishAILibraryInfo*>* GetSkirmishAIInfos() const = 0;
//	virtual const std::map<const SGAIKey, CGroupAILibraryInfo*>* GetGroupAIInfos() const = 0;
	typedef std::map<const SAIInterfaceSpecifyer, CAIInterfaceLibraryInfo*, SAIInterfaceSpecifyer_Comparator> T_interfaceInfos;
	typedef std::map<const SSAIKey, CSkirmishAILibraryInfo*, SSAIKey_Comparator> T_skirmishAIInfos;
	typedef std::map<const SGAIKey, CGroupAILibraryInfo*, SGAIKey_Comparator> T_groupAIInfos;
	
	virtual const T_interfaceInfos* GetInterfaceInfos() const = 0;
	virtual const T_skirmishAIInfos* GetSkirmishAIInfos() const = 0;
	virtual const T_groupAIInfos* GetGroupAIInfos() const = 0;

	virtual std::vector<SSAIKey> ResolveSkirmishAIKey(const SSAISpecifyer& skirmishAISpecifyer) const = 0;
	virtual std::vector<SSAIKey> ResolveSkirmishAIKey(const std::string& skirmishAISpecifyer) const = 0;
	// a Skirmish AI (its library) is only really loaded when it is not yet loaded.
	virtual const ISkirmishAILibrary* FetchSkirmishAILibrary(const SSAIKey& skirmishAIKey) = 0;
	// a Skirmish AI is only unloaded when ReleaseSkirmishAILibrary() is called
	// as many times as GetSkirmishAILibrary() was.
	// loading and unloading of the interfaces
	// is handled internally/automatically.
	virtual void ReleaseSkirmishAILibrary(const SSAIKey& skirmishAIKey) = 0;
	virtual void ReleaseAllSkirmishAILibraries() = 0; // unloads all currently Skirmish loaded AIs
	
	virtual std::vector<SGAIKey> ResolveGroupAIKey(const std::string& groupAISpecifyer) const = 0;
	// a Group AI (its library) is only really loaded when it is not yet loaded.
	virtual const IGroupAILibrary* FetchGroupAILibrary(const SGAIKey& groupAIKey) = 0;
	// a Group AI is only unloaded when ReleaseSkirmishAILibrary() is called
	// as many times as GetSkirmishAILibrary() was.
	// loading and unloading of the interfaces
	// is handled internally/automatically.
	virtual void ReleaseGroupAILibrary(const SGAIKey& groupAIKey) = 0;
	virtual void ReleaseAllGroupAILibraries() = 0; // unloads all currently loaded Group AIs
	
	virtual void ReleaseEverything() = 0; // unloads all currently loaded AIs and interfaces
	
public:
	/* guaranteed to not return NULL */
	static IAILibraryManager* GetInstance();
	static void OutputSkirmishAIInfo();
private:
	static IAILibraryManager* myAILibraryManager;
	
protected:
	static bool SplittAIKey(const std::string& key,
			std::string* aiName,
			std::string* aiVersion,
			std::string* interfaceName,
			std::string* interfaceVersion);
};

#endif	/* _IAILIBRARYMANAGER_H */
