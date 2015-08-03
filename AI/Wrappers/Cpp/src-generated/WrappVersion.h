/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPVERSION_H
#define _CPPWRAPPER_WRAPPVERSION_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Version.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappVersion : public Version {

private:
	int skirmishAIId;

	WrappVersion(int skirmishAIId);
	virtual ~WrappVersion();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Version* GetInstance(int skirmishAIId);

	/**
	 * Returns the major engine revision number (e.g. 83)
	 */
public:
	// @Override
	virtual const char* GetMajor();

	/**
	 * Minor version number (e.g. "5")
	 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
	 */
public:
	// @Override
	virtual const char* GetMinor();

	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
public:
	// @Override
	virtual const char* GetPatchset();

	/**
	 * SCM Commits version part (e.g. "" or "13")
	 * Number of commits since the last version tag.
	 * This matches the regex "[0-9]*".
	 */
public:
	// @Override
	virtual const char* GetCommits();

	/**
	 * SCM unique identifier for the current commit.
	 * This matches the regex "([0-9a-f]{6})?".
	 */
public:
	// @Override
	virtual const char* GetHash();

	/**
	 * SCM branch name (e.g. "master" or "develop")
	 */
public:
	// @Override
	virtual const char* GetBranch();

	/**
	 * Additional information (compiler flags, svn revision etc.)
	 */
public:
	// @Override
	virtual const char* GetAdditional();

	/**
	 * time of build
	 */
public:
	// @Override
	virtual const char* GetBuildTime();

	/**
	 * Returns whether this is a release build of the engine
	 */
public:
	// @Override
	virtual bool IsRelease();

	/**
	 * The basic part of a spring version.
	 * This may only be used for sync-checking if IsRelease() returns true.
	 * @return "Major.PatchSet" or "Major.PatchSet.1"
	 */
public:
	// @Override
	virtual const char* GetNormal();

	/**
	 * The sync relevant part of a spring version.
	 * This may be used for sync-checking through a simple string-equality test.
	 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
	 */
public:
	// @Override
	virtual const char* GetSync();

	/**
	 * The verbose, human readable version.
	 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
	 */
public:
	// @Override
	virtual const char* GetFull();
}; // class WrappVersion

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPVERSION_H

