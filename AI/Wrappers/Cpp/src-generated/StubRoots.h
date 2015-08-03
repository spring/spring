/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBROOTS_H
#define _CPPWRAPPER_STUBROOTS_H

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
class StubRoots : public Roots {

protected:
	virtual ~StubRoots();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the number of springs data dirs.
	 */
private:
	int size;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSize(int size);
	// @Override
	virtual int GetSize();
	/**
	 * Returns the data dir at dirIndex, which is valid between 0 and (DataDirs_Roots_getSize() - 1).
	 */
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
	// @Override
	virtual bool LocatePath(char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir);
	// @Override
	virtual char* AllocatePath(const char* const relPath, bool writeable, bool create, bool dir);
}; // class StubRoots

}  // namespace springai

#endif // _CPPWRAPPER_STUBROOTS_H

