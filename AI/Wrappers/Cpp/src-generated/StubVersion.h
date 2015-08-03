/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBVERSION_H
#define _CPPWRAPPER_STUBVERSION_H

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
class StubVersion : public Version {

protected:
	virtual ~StubVersion();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the major engine revision number (e.g. 83)
	 */
private:
	const char* major;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMajor(const char* major);
	// @Override
	virtual const char* GetMajor();
	/**
	 * Minor version number (e.g. "5")
	 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
	 */
private:
	const char* minor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinor(const char* minor);
	// @Override
	virtual const char* GetMinor();
	/**
	 * Clients that only differ in patchset can still play together.
	 * Also demos should be compatible between patchsets.
	 */
private:
	const char* patchset;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPatchset(const char* patchset);
	// @Override
	virtual const char* GetPatchset();
	/**
	 * SCM Commits version part (e.g. "" or "13")
	 * Number of commits since the last version tag.
	 * This matches the regex "[0-9]*".
	 */
private:
	const char* commits;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCommits(const char* commits);
	// @Override
	virtual const char* GetCommits();
	/**
	 * SCM unique identifier for the current commit.
	 * This matches the regex "([0-9a-f]{6})?".
	 */
private:
	const char* hash;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHash(const char* hash);
	// @Override
	virtual const char* GetHash();
	/**
	 * SCM branch name (e.g. "master" or "develop")
	 */
private:
	const char* branch;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBranch(const char* branch);
	// @Override
	virtual const char* GetBranch();
	/**
	 * Additional information (compiler flags, svn revision etc.)
	 */
private:
	const char* additional;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAdditional(const char* additional);
	// @Override
	virtual const char* GetAdditional();
	/**
	 * time of build
	 */
private:
	const char* buildTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBuildTime(const char* buildTime);
	// @Override
	virtual const char* GetBuildTime();
	/**
	 * Returns whether this is a release build of the engine
	 */
private:
	bool isRelease;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRelease(bool isRelease);
	// @Override
	virtual bool IsRelease();
	/**
	 * The basic part of a spring version.
	 * This may only be used for sync-checking if IsRelease() returns true.
	 * @return "Major.PatchSet" or "Major.PatchSet.1"
	 */
private:
	const char* normal;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNormal(const char* normal);
	// @Override
	virtual const char* GetNormal();
	/**
	 * The sync relevant part of a spring version.
	 * This may be used for sync-checking through a simple string-equality test.
	 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
	 */
private:
	const char* sync;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSync(const char* sync);
	// @Override
	virtual const char* GetSync();
	/**
	 * The verbose, human readable version.
	 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
	 */
private:
	const char* full;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFull(const char* full);
	// @Override
	virtual const char* GetFull();
}; // class StubVersion

}  // namespace springai

#endif // _CPPWRAPPER_STUBVERSION_H

