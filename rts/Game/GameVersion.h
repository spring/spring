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
 * - 0.83.0                (release or hotfix preparation)
 * - 0.83.0.2              (release or hotfix preparation)
 * - 0.83.7.1              (release or hotfix preparation)
 * - 0.83.7-.release       (release-candidate for the 0.83.7 release)
 * - 0.83.7-.develop       (main development version while the 0.83.7 release is in preparation)
 * - 0.83.7+.develop       (main development version after the 0.83.7 release)
 * - 0.83.7-.coolFeature   (featrue-branch version while the 0.83.7 release is in preparation)
 * - 0.83.7+.coolFeature   (featrue-branch version after the 0.83.7 release)
 */
namespace SpringVersion
{
	/**
	 * Major version number (e.g. "0.77")
	 * The feature integration version part.
	 * This should change roughtly every 6 months.
	 * This matches the regex "[0-9]+\.[0-9]+".
	 */
	extern const std::string& GetMajor();

	/**
	 * Minor version number (e.g. "5")
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
	 * Patch-set version part (e.g. "1" or "develop")
	 * Bug-fix version part, for changes which preserve sync between clients.
	 * Demos should also be compatible between patch-sets.
	 * This may be a pure number (matching "[0-9]+") only on the <i>master</i>
	 * and on <i>hotfix</i> branches, Clients with the same Major.Minor can
	 * still play together.
	 * It will equal the branch-name and thus match "[0-9a-zA-Z_]+" on any
	 * branches except <i>master</i> and <i>hotfix</i>.
	 * This matches the regex "[0-9]+|[0-9a-zA-Z_]+".
	 */
	extern const std::string& GetPatchSet();

	/// additional information (compiler flags, VCS revision etc.)
	extern const std::string& GetAdditional();

	/// time of build
	extern const std::string& GetBuildTime();

	/**
	 * The sync relevant part of a spring version.
	 * This may only be used for sync-checking if it matches "[0-9.]+",
	 * And thus is a release version.
	 * @return "Major.Minor"
	 * @see Major
	 * @see Minor
	 */
	extern const std::string& Get();

	/**
	 * The verbose, human readable version.
	 * @return "Major.Minor.Patchset (Additional)"
	 * @see Major
	 * @see Minor
	 * @see Patchset
	 * @see Additional
	 */
	extern const std::string& GetFull();
};

#endif // GAME_VERSION_H
