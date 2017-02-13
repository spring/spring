/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_VERSION_H
#define GAME_VERSION_H

#include <string>

/**
 * Allows fetching of all build-specific info.
 *
 * until 0.82:
 * - 0.82.0                (release)
 * - 0.82.0.2              (release)
 * - 0.82.7                (release)
 * - 0.82.7.1              (release)
 * - 0.82+.0               (development version from after 0.82.0)
 * - 0.82+.7.1             (development version from after 0.82.7.1)
 * since 0.83:
 * - 83.0                (release or hotfix preparation)
 * - 83.0.1              (main development version after the 0.83.0 release)
 */
namespace SpringVersion
{
	/**
	 * Major version number (e.g. "83")
	 * The feature integration version part.
	 * This should change roughtly every 1 till 6 months.
	 * This matches the regex "[0-9]+".
	 */
	extern const std::string& GetMajor();

	/**
	 * Minor version number (e.g. "5")
	 * @deprecated since 4. October 2011 (pre release 83), will always return "0"
	 * Bug-fix version part, for changes which break sync between clients.
	 * This may be a pure number (matching "[0-9]+") only on the <i>master</i>
	 * and on <i>hotfix</i> branches.
	 * It will match "[0-9]+[-]" on <i>release</i> branches and on
	 * <i>develop</i> (and <i>feature</i> branches) before a release.
	 * It will match "[0-9]+[+]" on <i>develop</i> (and <i>feature</i> branches)
	 * after a release.
	 * This matches the regex "[0-9]+[+-]?".
	 */
	extern const std::string& GetMinor();

	/**
	 * Patch-set version part (e.g. "0" or "2")
	 * Bug-fix version part, for changes which preserve sync between clients.
	 * Demos should also be compatible between patch-sets.
	 * This matches the regex "[0-9]+".
	 */
	extern const std::string& GetPatchSet();

	/**
	 * SCM Commits version part (e.g. "" or "13")
	 * Number of commits since the last version tag.
	 * This matches the regex "[0-9]*".
	 */
	extern const std::string& GetCommits();

	/**
	 * SCM unique identifier for the current commit.
	 * This matches the regex "([0-9a-f]{6})?".
	 */
	extern const std::string& GetHash();

	/**
	 * SCM branch name (e.g. "master" or "develop")
	 */
	extern const std::string& GetBranch();

	/// additional information (compiler flags, VCS revision etc.)
	extern const std::string& GetAdditional();

	/// build options
	extern const std::string& GetBuildEnvironment();

	/// compiler information
	extern const std::string& GetCompiler();

	/// Returns whether this is a release build of the engine
	extern bool IsRelease();

	/// Returns true if this build is a "HEADLESS" build
	extern bool IsHeadless();

	/// Returns true if this build is a "UNITSYNC" build
	extern bool IsUnitsync();

	/**
	 * The basic part of a spring version.
	 * This may only be used for sync-checking if IsRelease() returns true.
	 * @return "Major.PatchSet" or "Major.PatchSet.1"
	 * @see GetSync
	 */
	extern const std::string& Get();

	/**
	 * The sync relevant part of a spring version.
	 * This may be used for sync-checking through a simple string-equality test.
	 * In essence this means, that only releases with the same Major release
	 * number may be detected as syncing.
	 * @return "Major" or "Major.PatchSet.1-Commits-gHash Branch"
	 */
	extern const std::string& GetSync();

	/**
	 * The verbose, human readable version.
	 * @return "Major.Patchset[.1-Commits-gHash Branch] (Additional)"
	 * @see GetMajor
	 * @see GetPatchSet
	 * @see GetCommits
	 * @see GetHash
	 * @see GetBranch
	 * @see GetAdditional
	 */
	extern const std::string& GetFull();
}

#endif // GAME_VERSION_H
