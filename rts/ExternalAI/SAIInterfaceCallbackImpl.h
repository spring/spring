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

#ifndef _SAIINTERFACECALLBACKIMPL_H
#define _SAIINTERFACECALLBACKIMPL_H

// Doc-comments for the functions in this header can be found in this file
#include "Interface/SAIInterfaceCallback.h"

EXPORT(int              ) aiInterfaceCallback_Engine_AIInterface_ABIVersion_getFailPart(int UNUSED_interfaceId);
EXPORT(int              ) aiInterfaceCallback_Engine_AIInterface_ABIVersion_getWarningPart(int UNUSED_interfaceId);

EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getMajor(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getMinor(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getPatchset(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getAdditional(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getBuildTime(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getNormal(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_Engine_Version_getFull(int UNUSED_interfaceId);


EXPORT(int              ) aiInterfaceCallback_AIInterface_Info_getSize(int interfaceId);
EXPORT(const char* const) aiInterfaceCallback_AIInterface_Info_getKey(int interfaceId, int infoIndex);
EXPORT(const char* const) aiInterfaceCallback_AIInterface_Info_getValue(int interfaceId, int infoIndex);
EXPORT(const char* const) aiInterfaceCallback_AIInterface_Info_getDescription(int interfaceId, int infoIndex);
EXPORT(const char* const) aiInterfaceCallback_AIInterface_Info_getValueByKey(int interfaceId, const char* const key);

EXPORT(int              ) aiInterfaceCallback_Teams_getSize(int UNUSED_interfaceId);

EXPORT(int              ) aiInterfaceCallback_SkirmishAIs_getSize(int UNUSED_interfaceId);
EXPORT(int              ) aiInterfaceCallback_SkirmishAIs_getMax(int UNUSED_interfaceId);
EXPORT(const char* const) aiInterfaceCallback_SkirmishAIs_Info_getValueByKey(int UNUSED_interfaceId, const char* const shortName, const char* const version, const char* const key);

EXPORT(void             ) aiInterfaceCallback_Log_log(int interfaceId, const char* const msg);
EXPORT(void             ) aiInterfaceCallback_Log_exception(int interfaceId, const char* const msg, int severety, bool die);

EXPORT(char             ) aiInterfaceCallback_DataDirs_getPathSeparator(int UNUSED_interfaceId);
EXPORT(int              ) aiInterfaceCallback_DataDirs_Roots_getSize(int UNUSED_interfaceId);
EXPORT(bool             ) aiInterfaceCallback_DataDirs_Roots_getDir(int UNUSED_interfaceId, char* path, int path_sizeMax, int dirIndex);
EXPORT(bool             ) aiInterfaceCallback_DataDirs_Roots_locatePath(int UNUSED_interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(char*            ) aiInterfaceCallback_DataDirs_Roots_allocatePath(int UNUSED_interfaceId, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(const char* const) aiInterfaceCallback_DataDirs_getConfigDir(int interfaceId);
EXPORT(bool             ) aiInterfaceCallback_DataDirs_locatePath(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(char*            ) aiInterfaceCallback_DataDirs_allocatePath(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(const char* const) aiInterfaceCallback_DataDirs_getWriteableDir(int interfaceId);


#if defined __cplusplus && !defined BUILDING_AI
class CAIInterfaceLibraryInfo;

// for engine internal use only
int  aiInterfaceCallback_getInstanceFor(const CAIInterfaceLibraryInfo* info, struct SAIInterfaceCallback* callback);
void aiInterfaceCallback_release(int interfaceId);
#endif // defined __cplusplus && !defined BUILDING_AI

#endif // _SAIINTERFACECALLBACKIMPL_H
