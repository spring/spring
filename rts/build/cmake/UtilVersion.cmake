# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# Spring version related CMake utilities
# --------------------------------------
#
# Functions and macros defined in this file:
# * ParseSpringVersion        - Parses a Spring version string into one var for each part of the version
# * CreateSpringVersionString - Concatenates Spring version string parts to form a full version specifier
# * CheckSpringReleaseVersion - Checks whether a given string is a release version
# * GetVersionFromFile        - Retrieves the version from the VERSION file
# * FetchSpringVersion        - Retrieves the version either from git, or from the VERSION file, in case we are not in a git repo (tarball)
#

Include(Util)
Include(UtilGit)


Set(D10 "[0-9]") # matches a decimal digit
Set(D16 "[0-9a-f]") # matches a (lower-case) hexadecimal digit

# Matches the engine major version part
Set(VERSION_REGEX_MAJOR "${D10}?${D10}?${D10}[.]${D10}?${D10}?${D10}")
Set(VERSION_REGEX_MAJOR_MATCH_EXAMPLES "\"0.0\", \"0.82\"")

# Matches the engine minor version part (release)
Set(VERSION_REGEX_MINOR_RELEASE "${D10}?${D10}?${D10}")
Set(VERSION_REGEX_MINOR_RELEASE_MATCH_EXAMPLES "\"0\", \"7\"")
# Matches the engine minor version part (any)
Set(VERSION_REGEX_MINOR_ANY "${VERSION_REGEX_MINOR_RELEASE}[+-]?")
Set(VERSION_REGEX_MINOR_ANY_MATCH_EXAMPLES "\"0\", \"7\", \"4-\", \"2+\"")

# Matches the engine patchSet version part for release versions
Set(VERSION_REGEX_PATCH_RELEASE "${D10}?${D10}?${D10}")
Set(VERSION_REGEX_PATCH_RELEASE_MATCH_EXAMPLES "\"0\", \"5\"")

# Matches the engine patchSet version part for any versions
Set(VERSION_REGEX_PATCH_ANY "${VERSION_REGEX_PATCH_RELEASE}|[0-9a-zA-Z_]+")
Set(VERSION_REGEX_PATCH_ANY_MATCH_EXAMPLES "\"0\", \"5\", \"develop\", \"release\", \"myCoolNewFeature_algorithm66\"")

# Matches the engine non-release postfix ("<#commits>-g<SHA1>"" version part
Set(VERSION_REGEX_NON_RELEASE_POSTFIX "-(${D10}+)-g(${D16}${D16}${D16}${D16}${D16}${D16}${D16})")


# Matches engine release version strings
# Match-groups (Caution: not consecutive! example input: "0.82.7.1"):
# \\1 : Major version, for example "0.82"
# \\2 : Minor version, for example "7"
# \\4 : Patchset, for example "1", may be ""
Set(VERSION_REGEX_RELEASE "(${VERSION_REGEX_MAJOR})[.](${VERSION_REGEX_MINOR_RELEASE})([.](${VERSION_REGEX_PATCH_RELEASE}))?")
Set(VERSION_REGEX_RELEASE_MATCH_EXAMPLES "\"0.82.7\", \"0.82.7.1\"")
Set(VERSION_REGEX_ANY "(${VERSION_REGEX_MAJOR})[.](${VERSION_REGEX_MINOR_ANY})([.](${VERSION_REGEX_PATCH_ANY}))?")
Set(VERSION_REGEX_ANY_MATCH_EXAMPLES "\"0.82.7\", \"0.82.7.1\", \"0.82.7+.develop\", \"0.82.7-.release\"")


# Matches engine release & non-release version strings
# Match-groups (Caution: not consecutive! example input: "0.82.7.1-2302-g6d3a71e"):
# \\1 : Major version, for example "0.82"
# \\2 : Minor version, for example "7"
# \\4 : Patchset, for example "1", may be ""
# \\6 : Commits since last release, for example "2302", may be ""
# \\7 : First 7 digits of the current commit's SHA1, for example "6d3a71e", may be ""
Set(VERSION_REGEX_ALL "${VERSION_REGEX_ANY}(${VERSION_REGEX_NON_RELEASE_POSTFIX})?")
Set(VERSION_REGEX_ALL_MATCH_EXAMPLES "\"0.82.7\", \"0.82.7.1\", \"0.82.7.develop\", \"0.82.7.release\", \"0.82.7.develop-2302-g6d3a71e\"")


# Parses a Spring version string into one var for each part of the version.
# @see CreateSpringVersionString
# sample version: "0.82.7.1-2302-g6d3a71e"
# sample output:
#   - ${varPrefix}_MAJOR     "0.82"
#   - ${varPrefix}_MINOR     "7"
#   - ${varPrefix}_PATCH_SET "1"
#   - ${varPrefix}_COMMITS   "2302"
#   - ${varPrefix}_HASH      "6d3a71e"
Macro    (ParseSpringVersion varPrefix version)
	#Set(VERSION_REGEX_RELEASE_MAYBE_PATCH "(${VERSION_REGEX_MAJOR})[.](${VERSION_REGEX_MINOR_ANY})([-.].*)?")
	Set(VERSION_REGEX_RELEASE_PATCH "(${VERSION_REGEX_MAJOR})[.](${VERSION_REGEX_MINOR_ANY})[.](${VERSION_REGEX_PATCH_ANY})([-].*)?")
	Set(VERSION_REGEX_NON_RELEASE   "${VERSION_REGEX_ANY}${VERSION_REGEX_NON_RELEASE_POSTFIX}")
#Message("VERSION_REGEX_NON_RELEASE: ${VERSION_REGEX_NON_RELEASE}")
	CatchRegexGroup("${VERSION_REGEX_ALL}"           1 "${varPrefix}_MAJOR"     "${version}")
	CatchRegexGroup("${VERSION_REGEX_ALL}"           2 "${varPrefix}_MINOR"     "${version}")
	CatchRegexGroup("${VERSION_REGEX_RELEASE_PATCH}" 3 "${varPrefix}_PATCH_SET" "${version}")
	CatchRegexGroup("${VERSION_REGEX_NON_RELEASE}"   5 "${varPrefix}_COMMITS"   "${version}")
	CatchRegexGroup("${VERSION_REGEX_NON_RELEASE}"   6 "${varPrefix}_HASH"      "${version}")
EndMacro (ParseSpringVersion)

Macro    (PrintParsedSpringVersion varPrefix)
	Message("  major:     ${${varPrefix}_MAJOR}")
	Message("  minor:     ${${varPrefix}_MINOR}")
	Message("  patch-set: ${${varPrefix}_PATCH_SET}")
	Message("  commits:   ${${varPrefix}_COMMITS}")
	Message("  hash:      ${${varPrefix}_HASH}")
EndMacro (PrintParsedSpringVersion)


# Concatenates Spring version string parts to form a full version specifier.
# @see ParseSpringVersion
# sample input:
#   - major    "0.82"
#   - minor    "7"
#   - patchSet "1"
#   - commits  "2302"
#   - hash     "6d3a71e"
# sample output: "0.82.7.1-2302-g6d3a71e"
Macro    (CreateSpringVersionString res_var major minor patchSet commits hash)
	Set(${res_var} "${major}.${minor}")
	If     (NOT "${patchSet}" STREQUAL "" AND NOT "${patchSet}" STREQUAL "0")
		Set(${res_var} "${${res_var}}.${patchSet}")
	EndIf  ()
	If     (NOT "${commits}" STREQUAL "")
		Set(${res_var} "${${res_var}}-${commits}-g${hash}")
	EndIf  ()
EndMacro (CreateSpringVersionString)





# Sets res_var to TRUE if version is a Spring release version specifier,
# as oposed to a non-release/development version.
Macro    (CheckSpringReleaseVersion res_var version)
	Set(${res_var} FALSE)
	If     ("${version}" MATCHES "^${VERSION_REGEX_RELEASE}$")
		Set(${res_var} TRUE)
	EndIf  ()
EndMacro (CheckSpringReleaseVersion)




# Gets the version from a text file.
# (actually just reads the text file content into a variable)
Macro    (GetVersionFromFile vers_var vers_file)
	# unset the vars
	Set(${vers_var})
	Set(${vers_var}-NOTFOUND)

	If    (EXISTS "${vers_file}")
		File(STRINGS "${vers_file}" ${vers_var}_tmp LIMIT_COUNT 1 REGEX "^${VERSION_REGEX_ALL}$")
		If    (NOT ("${${vers_var}_tmp}" STREQUAL ""))
			Set(${vers_var} "${${vers_var}_tmp}")
		Else  ()
			Set(${vers_var}-NOTFOUND "1")
		EndIf ()
	Else  (EXISTS "${vers_file}")
		#Set(${vers_var} "UNKNOWN_VERSION")
		Set(${vers_var}-NOTFOUND "1")
	EndIf (EXISTS "${vers_file}")
EndMacro (GetVersionFromFile)





# Gets the version for a source directory, either from GIT,
# or if that fails (for example if it is not a git repository,
# as is the case when using a tarball), from the VERSION file.
# Creates a FATAL_ERROR on failure.
# Sets the following vars:
# - ${prefix}_VERSION
Macro    (FetchSpringVersion dir prefix)
	# unset the vars
	Set(${prefix}_VERSION)
	Set(${prefix}_VERSION-NOTFOUND)

	If     (EXISTS "${dir}/.git")
		# Try to fetch version through git
		If     (NOT GIT_FOUND)
			Message(FATAL_ERROR "Git repository detected, but git executable not found; failed to fetch ${prefix} version.")
		EndIf  (NOT GIT_FOUND)

		# Fetch git version info
		Git_Util_Branch(${prefix}_Branch ${dir})
		If     (NOT ${prefix}_Branch)
			Message(FATAL_ERROR "Failed to fetch the git branch for ${prefix}.")
		EndIf  (NOT ${prefix}_Branch)
		If     (NOT "${${prefix}_Branch}" STREQUAL "master")
			# We always want the long git-describe output on non-releases
			# for example: 0.83.0-.develop-0-g1234567
			Set(${prefix}_Describe_Args --long)
		EndIf  (NOT "${${prefix}_Branch}" STREQUAL "master")
		Git_Util_Describe(${prefix}_Describe ${dir} "*.*.*" ${${prefix}_Describe_Args})
		If     (NOT ${prefix}_Describe)
			Message(FATAL_ERROR "Failed to fetch git-describe for ${prefix}.")
		EndIf  (NOT ${prefix}_Describe)

		# Parse the version into subparts, and modify it
		ParseSpringVersion(${prefix} "${${prefix}_Describe}")
		If     ("${${prefix}_Branch}" STREQUAL "master")
			If     (NOT "${${prefix}_COMMITS}" STREQUAL "" OR NOT "${${prefix}_HASH}" STREQUAL "")
				Message(AUTHOR_WARNING "Commit without a version tag found on branch master for ${prefix}; this indicates a tagging/branching/push error.")
			EndIf  (NOT "${${prefix}_COMMITS}" STREQUAL "" OR NOT "${${prefix}_HASH}" STREQUAL "")
		Else   ("${${prefix}_Branch}" STREQUAL "master")
			If     (NOT "${${prefix}_PATCH_SET}" STREQUAL "")
				Message(AUTHOR_WARNING "Patch-Set found in version-string on non-master branch for ${prefix}; this indicates a tagging/branching error.")
			EndIf  (NOT "${${prefix}_PATCH_SET}" STREQUAL "")
			Set(${prefix}_PATCH_SET "${${prefix}_Branch}")
		EndIf  ("${${prefix}_Branch}" STREQUAL "master")

		# Recreate the modified version specifier
		CreateSpringVersionString(${prefix}_VERSION
				"${${prefix}_MAJOR}"
				"${${prefix}_MINOR}"
				"${${prefix}_PATCH_SET}"
				"${${prefix}_COMMITS}"
				"${${prefix}_HASH}"
		)
	Else   (EXISTS "${dir}/.git")
		# Try to fetch version through VERSION file
		GetVersionFromFile(${prefix}_VERSION "${dir}/VERSION")
		If    (NOT "${${prefix}_VERSION}" STREQUAL "UNKNOWN_VERSION")
			Message(STATUS "${prefix} version fetched from VERSION file: ${${prefix}_VERSION}")
		Else  (NOT "${${prefix}_VERSION}" STREQUAL "UNKNOWN_VERSION")
			Message(FATAL_ERROR "Failed to fetch ${prefix} version.")
		EndIf (NOT "${${prefix}_VERSION}" STREQUAL "UNKNOWN_VERSION")
	EndIf  (EXISTS "${dir}/.git")
EndMacro (FetchSpringVersion)


