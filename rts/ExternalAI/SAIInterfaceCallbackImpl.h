/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S_AI_INTERFACE_CALLBACK_IMPL_H
#define S_AI_INTERFACE_CALLBACK_IMPL_H

// Doc-comments for the functions in this header can be found in this file
#include "Interface/SAIInterfaceCallback.h"

EXPORT(int              ) aiInterfaceCallback_Engine_AIInterface_ABIVersion_getFailPart(int UNUSED_interfaceId);
EXPORT(int              ) aiInterfaceCallback_Engine_AIInterface_ABIVersion_getWarningPart(int UNUSED_interfaceId);

EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getMajor(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getMinor(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getPatchset(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getCommits(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getHash(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getBranch(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getAdditional(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getBuildTime(int UNUSED_interfaceId);
EXPORT(bool             ) aiInterfaceCallback_Engine_Version_isRelease(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getNormal(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getSync(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_Engine_Version_getFull(int UNUSED_interfaceId);


EXPORT(int              ) aiInterfaceCallback_AIInterface_Info_getSize(int interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_AIInterface_Info_getKey(int interfaceId, int infoIndex);
EXPORT(const char*      ) aiInterfaceCallback_AIInterface_Info_getValue(int interfaceId, int infoIndex);
EXPORT(const char*      ) aiInterfaceCallback_AIInterface_Info_getDescription(int interfaceId, int infoIndex);
EXPORT(const char*      ) aiInterfaceCallback_AIInterface_Info_getValueByKey(int interfaceId, const char* const key);

EXPORT(int              ) aiInterfaceCallback_Teams_getSize(int UNUSED_interfaceId);

EXPORT(int              ) aiInterfaceCallback_SkirmishAIs_getSize(int UNUSED_interfaceId);
EXPORT(int              ) aiInterfaceCallback_SkirmishAIs_getMax(int UNUSED_interfaceId);
EXPORT(const char*      ) aiInterfaceCallback_SkirmishAIs_Info_getValueByKey(int UNUSED_interfaceId, const char* const shortName, const char* const version, const char* const key);

EXPORT(void             ) aiInterfaceCallback_Log_log(int interfaceId, const char* const msg);
EXPORT(void             ) aiInterfaceCallback_Log_logsl(int interfaceId, const char* section, int loglevel, const char* const msg);
EXPORT(void             ) aiInterfaceCallback_Log_exception(int interfaceId, const char* const msg, int severety, bool die);

EXPORT(char             ) aiInterfaceCallback_DataDirs_getPathSeparator(int UNUSED_interfaceId);
EXPORT(int              ) aiInterfaceCallback_DataDirs_Roots_getSize(int UNUSED_interfaceId);
EXPORT(bool             ) aiInterfaceCallback_DataDirs_Roots_getDir(int UNUSED_interfaceId, char* path, int path_sizeMax, int dirIndex);
EXPORT(bool             ) aiInterfaceCallback_DataDirs_Roots_locatePath(int UNUSED_interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(char*            ) aiInterfaceCallback_DataDirs_Roots_allocatePath(int UNUSED_interfaceId, const char* const relPath, bool writeable, bool create, bool dir);
EXPORT(const char*      ) aiInterfaceCallback_DataDirs_getConfigDir(int interfaceId);
EXPORT(bool             ) aiInterfaceCallback_DataDirs_locatePath(int interfaceId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);
EXPORT(char*            ) aiInterfaceCallback_DataDirs_allocatePath(int interfaceId, const char* const relPath, bool writeable, bool create, bool dir, bool common);
EXPORT(const char*      ) aiInterfaceCallback_DataDirs_getWriteableDir(int interfaceId);


#if defined __cplusplus && !defined BUILDING_AI
class CAIInterfaceLibraryInfo;

// for engine internal use only
int  aiInterfaceCallback_getInstanceFor(const CAIInterfaceLibraryInfo* info, struct SAIInterfaceCallback* callback);
void aiInterfaceCallback_release(int interfaceId);
#endif // defined __cplusplus && !defined BUILDING_AI

#endif // S_AI_INTERFACE_CALLBACK_IMPL_H
