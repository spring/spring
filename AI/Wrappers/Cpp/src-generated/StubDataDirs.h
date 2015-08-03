/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBDATADIRS_H
#define _CPPWRAPPER_STUBDATADIRS_H

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
class StubDataDirs : public DataDirs {

protected:
	virtual ~StubDataDirs();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns '/' on posix and '\\' on windows
	 */
private:
	char pathSeparator;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPathSeparator(char pathSeparator);
	// @Override
	virtual char GetPathSeparator();
	/**
	 * This interfaces main data dir, which is where the shared library
	 * and the InterfaceInfo.lua file are located, e.g.:
	 * /usr/share/games/spring/AI/Skirmish/RAI/0.601/
	 */
private:
	const char* configDir;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetConfigDir(const char* configDir);
	// @Override
	virtual const char* GetConfigDir();
	/**
	 * This interfaces writable data dir, which is where eg logs, caches
	 * and learning data should be stored, e.g.:
	 * /home/userX/.spring/AI/Skirmish/RAI/0.601/
	 */
private:
	const char* writeableDir;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWriteableDir(const char* writeableDir);
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
	// @Override
	virtual bool LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common);
	/**
	 * @see     locatePath()
	 */
	// @Override
	virtual char* AllocatePath(const char* const relPath, bool writeable, bool create, bool dir, bool common);
private:
	springai::Roots* roots;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRoots(springai::Roots* roots);
	// @Override
	virtual springai::Roots* GetRoots();
}; // class StubDataDirs

}  // namespace springai

#endif // _CPPWRAPPER_STUBDATADIRS_H

