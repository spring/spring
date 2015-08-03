/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_VERSION_H
#define _CPPWRAPPER_VERSION_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Version {

public:
	virtual ~Version(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the major engine revision number (e.g. 83)
	 */
public:
	virtual const char* GetMajor() = 0;

	/**
	 * Minor version number (e.g. "5")
	 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
	 */
public:
	virtual const char* GetMinor() = 0;

	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
public:
	virtual const char* GetPatchset() = 0;

	/**
	 * SCM Commits version part (e.g. "" or "13")
	 * Number of commits since the last version tag.
	 * This matches the regex "[0-9]*".
	 */
public:
	virtual const char* GetCommits() = 0;

	/**
	 * SCM unique identifier for the current commit.
	 * This matches the regex "([0-9a-f]{6})?".
	 */
public:
	virtual const char* GetHash() = 0;

	/**
	 * SCM branch name (e.g. "master" or "develop")
	 */
public:
	virtual const char* GetBranch() = 0;

	/**
	 * Additional information (compiler flags, svn revision etc.)
	 */
public:
	virtual const char* GetAdditional() = 0;

	/**
	 * time of build
	 */
public:
	virtual const char* GetBuildTime() = 0;

	/**
	 * Returns whether this is a release build of the engine
	 */
public:
	virtual bool IsRelease() = 0;

	/**
	 * The basic part of a spring version.
	 * This may only be used for sync-checking if IsRelease() returns true.
	 * @return "Major.PatchSet" or "Major.PatchSet.1"
	 */
public:
	virtual const char* GetNormal() = 0;

	/**
	 * The sync relevant part of a spring version.
	 * This may be used for sync-checking through a simple string-equality test.
	 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
	 */
public:
	virtual const char* GetSync() = 0;

	/**
	 * The verbose, human readable version.
	 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
	 */
public:
	virtual const char* GetFull() = 0;

}; // class Version

}  // namespace springai

#endif // _CPPWRAPPER_VERSION_H

