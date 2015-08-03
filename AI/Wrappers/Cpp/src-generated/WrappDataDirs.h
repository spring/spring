/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPDATADIRS_H
#define _CPPWRAPPER_WRAPPDATADIRS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "DataDirs.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappDataDirs : public DataDirs {

private:
	int skirmishAIId;

	WrappDataDirs(int skirmishAIId);
	virtual ~WrappDataDirs();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static DataDirs* GetInstance(int skirmishAIId);

	/**
	 * Returns '/' on posix and '\\' on windows
	 */
public:
	// @Override
	virtual char GetPathSeparator();

	/**
	 * This interfaces main data dir, which is where the shared library
	 * and the InterfaceInfo.lua file are located, e.g.:
	 * /usr/share/games/spring/AI/Skirmish/RAI/0.601/
	 */
public:
	// @Override
	virtual const char* GetConfigDir();

	/**
	 * This interfaces writable data dir, which is where eg logs, caches
	 * and learning data should be stored, e.g.:
	 * /home/userX/.spring/AI/Skirmish/RAI/0.601/
	 */
public:
	// @Override
	virtual const char* GetWriteableDir();

	/**
	 * Returns an absolute path which consists of:
	 * data-dir + Skirmish-AI-path + relative-path.
	 * 
	 * example:
	 * input:  "log/main.log", writeable, create, !dir, !common
	 * output: "/home/userX/.spring/AI/Skirmish/RAI/0.601/log/main.log"
	 * The path "/home/userX/.spring/AI/Skirmish/RAI/0.601/log/" is created,
	 * if it does not yet exist.
	 * 
	 * @see DataDirs_Roots_locatePath
	 * @param   path          store for the resulting absolute path
	 * @param   path_sizeMax  storage size of the above
	 * @param   writeable  if true, only the writable data-dir is considered
	 * @param   create     if true, and realPath is not found, its dir structure
	 *                     is created recursively under the writable data-dir
	 * @param   dir        if true, realPath specifies a dir, which means if
	 *                     create is true, the whole path will be created,
	 *                     including the last part
	 * @param   common     if true, the version independent data-dir is formed,
	 *                     which uses "common" instead of the version, eg:
	 *                     "/home/userX/.spring/AI/Skirmish/RAI/common/..."
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
public:
	// @Override
	virtual bool LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);

	/**
	 * @see     locatePath()
	 */
public:
	// @Override
	virtual char* AllocatePath(const char* const relPath, bool writeable, bool create, bool dir, bool common);

public:
	// @Override
	virtual springai::Roots* GetRoots();
}; // class WrappDataDirs

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPDATADIRS_H

