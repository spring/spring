/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPROOTS_H
#define _CPPWRAPPER_WRAPPROOTS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Roots.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappRoots : public Roots {

private:
	int skirmishAIId;

	WrappRoots(int skirmishAIId);
	virtual ~WrappRoots();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Roots* GetInstance(int skirmishAIId);

	/**
	 * Returns the number of springs data dirs.
	 */
public:
	// @Override
	virtual int GetSize();

	/**
	 * Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1).
	 */
public:
	// @Override
	virtual bool GetDir(char* path, int path_sizeMax, int dirIndex);

	/**
	 * Returns an absolute path which consists of:
	 * data-dir + relative-path.
	 * 
	 * example:
	 * input:  "AI/Skirmish", writeable, create, dir
	 * output: "/home/userX/.spring/AI/Skirmish/"
	 * The path "/home/userX/.spring/AI/Skirmish/" is created,
	 * if it does not yet exist.
	 * 
	 * @see DataDirs_locatePath
	 * @param   path          store for the resulting absolute path
	 * @param   path_sizeMax  storage size of the above
	 * @param   relPath    the relative path to find
	 * @param   writeable  if true, only the writable data-dir is considered
	 * @param   create     if true, and realPath is not found, its dir structure
	 *                     is created recursively under the writable data-dir
	 * @param   dir        if true, realPath specifies a dir, which means if
	 *                     create is true, the whole path will be created,
	 *                     including the last part
	 * @return  whether the locating process was successfull
	 *          -> the path exists and is stored in an absolute form in path
	 */
public:
	// @Override
	virtual bool LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);

public:
	// @Override
	virtual char* AllocatePath(const char* const relPath, bool writeable, bool create, bool dir);
}; // class WrappRoots

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPROOTS_H

