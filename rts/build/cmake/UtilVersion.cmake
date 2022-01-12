# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# Spring version related CMake utilities
# --------------------------------------
#
# Functions and macros defined in this file:
# * parse_spring_version        - Parses a Spring version string into one var for each part of the version
# * create_spring_version_string - Concatenates Spring version string parts to form a full version specifier
# * check_spring_release_version - Checks whether a given string is a release version
# * get_version_from_file        - Retrieves the version from the VERSION file
# * fetch_spring_version        - Retrieves the version either from git, or from the VERSION file, in case we are not in a git repo (tarball)
#

Include(Util)
Include(UtilGit)


Set(D10 "[0-9]") # matches a decimal digit
Set(D16 "[0-9a-f]") # matches a (lower-case) hexadecimal digit

# Matches the engine major version part
# Releases that do NOT preserve sync show a change here (see release branch)
Set(VERSION_REGEX_MAJOR "${D10}+")
Set(VERSION_REGEX_MAJOR_MATCH_EXAMPLES "\"83\", \"90\", \"999\"")


# Matches the engine patchSet version part
# Releases that preserve sync show a change here (see hotfix branch)
Set(VERSION_REGEX_PATCH "${D10}+")
Set(VERSION_REGEX_PATCH_MATCH_EXAMPLES "\"0\", \"5\", \"999\"")

# Matches the engine dev version postfix (".1-<#commits>-g<SHA1> <branch>")
Set(VERSION_REGEX_DEV_POSTFIX "[.]1-(${D10}+)-g(${D16}+) ([^ ]+)")
Set(VERSION_REGEX_DEV_POSTFIX_MATCH_EXAMPLES "\".1-13-g1234aaf develop\", \".1-1354-g1234567 release\"")


# Matches engine release version strings
# Match-groups (Caution: not consecutive! example input: "0.82.7.1"):
# \\1 : Major version, for example "83"
# \\2 : Minor version, for example "0"
# \\3 : Commits since last release, for example "2302"
# \\4 : First 7 digits of the current commit's SHA1, for example "6d3a71e"
# \\5 : Git branch, for example "develop"
Set(VERSION_REGEX_RELEASE "(${VERSION_REGEX_MAJOR})[.](${VERSION_REGEX_PATCH})")
Set(VERSION_REGEX_RELEASE_MATCH_EXAMPLES "\"83.0\", \"84.1\"")
Set(VERSION_REGEX_DEV "${VERSION_REGEX_RELEASE}${VERSION_REGEX_DEV_POSTFIX}")
Set(VERSION_REGEX_DEV_MATCH_EXAMPLES "\"83.0.1-13-g1234aaf develop\", \"84.1.1-1354-g1234567 release\"")
Set(VERSION_REGEX_ANY "${VERSION_REGEX_RELEASE}(${VERSION_REGEX_DEV_POSTFIX})?")
Set(VERSION_REGEX_ANY_MATCH_EXAMPLES "83.0" "84.1" "83.0.1-13-g1234aaf develop" "84.1.1-1354-g1234567 release" "98.0.1-847-g61dee311 develop")


# Parses a Spring version string into one var for each part of the version.
# @see create_spring_version_string
# sample version: "83.2.1-2302-g6d3a71e develop"
# sample output:
#   - ${varPrefix}_MAJOR     "83"
#   - ${varPrefix}_PATCH_SET "2"
#   - ${varPrefix}_COMMITS   "2302"
#   - ${varPrefix}_HASH      "6d3a71e"
#   - ${varPrefix}_BRANCH    "develop"
macro (parse_spring_version varPrefix version)
	catch_regex_group("${VERSION_REGEX_ANY}" 1 "${varPrefix}_MAJOR"     "${version}")
	catch_regex_group("${VERSION_REGEX_ANY}" 2 "${varPrefix}_PATCH_SET" "${version}")
	catch_regex_group("${VERSION_REGEX_DEV}" 3 "${varPrefix}_COMMITS"   "${version}")
	catch_regex_group("${VERSION_REGEX_DEV}" 4 "${varPrefix}_HASH"      "${version}")
	catch_regex_group("${VERSION_REGEX_DEV}" 5 "${varPrefix}_BRANCH"    "${version}")
endmacro ()

macro (PrintParsedSpringVersion varPrefix)
	message ("  major:     ${${varPrefix}_MAJOR}")
	message ("  patch-set: ${${varPrefix}_PATCH_SET}")
	message ("  commits:   ${${varPrefix}_COMMITS}")
	message ("  hash:      ${${varPrefix}_HASH}")
	message ("  branch:    ${${varPrefix}_BRANCH}")
endmacro ()


# Concatenates Spring version string parts to form a full version specifier.
# @see parse_spring_version
# sample input:
#   - major    "0.82"
#   - minor    "7"
#   - patchSet "1"
#   - commits  "2302"
#   - hash     "6d3a71e"
# sample output: "0.82.7.1-2302-g6d3a71e"
macro (create_spring_version_string res_var major patchSet commits hash branch)
	Set(${res_var} "${major}.${patchSet}")
	if (NOT "${commits}" STREQUAL "")
		Set(${res_var} "${${res_var}}-${commits}-g${hash} ${branch}")
	endif ()
endmacro ()





# Sets res_var to TRUE if version is a Spring release version specifier,
# as oposed to a non-release/development version.
macro (check_spring_release_version res_var version)
	Set(${res_var} FALSE)
	if ("${version}" MATCHES "^${VERSION_REGEX_RELEASE}$")
		Set(${res_var} TRUE)
	endif ()
endmacro ()




# Gets the version from a text file.
# (actually just reads the text file content into a variable)
macro (get_version_from_file vers_var vers_file)
	# unset the vars
	Set(${vers_var})
	Set(${vers_var}-NOTFOUND)

	if (EXISTS "${vers_file}")
		File(STRINGS "${vers_file}" ${vers_var}_tmp LIMIT_COUNT 1 REGEX "^${VERSION_REGEX_ANY}$")
		if (NOT "${${vers_var}_tmp}" STREQUAL "")
			Set(${vers_var} "${${vers_var}_tmp}")
		else ()
			Set(${vers_var}-NOTFOUND "1")
		endif ()
	else ()
		Set(${vers_var}-NOTFOUND "1")
	endif ()
endmacro ()





# Gets the version for a source directory, either from GIT,
# or if that fails (for example if it is not a git repository,
# as is the case when using a tarball), from the VERSION file.
# Creates a FATAL_ERROR on failure.
# Sets the following vars:
# - ${prefix}_VERSION
macro (fetch_spring_version dir prefix)
	# unset the vars
	Set(${prefix}_VERSION)
	Set(${prefix}_VERSION-NOTFOUND)

	if (EXISTS "${dir}/.git")
		# Try to fetch version through git
		if (NOT GIT_FOUND)
			message (FATAL_ERROR "Git repository detected, but git executable not found; failed to fetch ${prefix} version.")
		endif ()

		# Fetch git version info
		git_util_describe(${prefix}_Describe ${dir} "*")
		if (NOT ${prefix}_Describe)
			message (FATAL_ERROR "Failed to fetch git-describe for ${prefix}.")
		endif ()
		if ("${${prefix}_Describe}" MATCHES "^${VERSION_REGEX_RELEASE}$")
			Set(${prefix}_IsRelease TRUE)
		else ()
			Set(${prefix}_IsRelease FALSE)
		endif ()
		if (NOT ${prefix}_IsRelease)
			# We always want the long git-describe output on non-releases
			# for example: 83.0.1-0-g1234567
			git_util_describe(${prefix}_Describe ${dir} "*" --long)
		endif ()

		Git_Util_Branch(${prefix}_Branch ${dir})
		if (${prefix}_IsRelease)
			Set(${prefix}_VERSION "${${prefix}_Describe}")
		else ()
			if (NOT ${prefix}_Branch)
				message (FATAL_ERROR "Failed to fetch the git branch for ${prefix}.")
			endif ()
			Set(${prefix}_VERSION "${${prefix}_Describe} ${${prefix}_Branch}")
		endif ()
		parse_spring_version(${prefix} "${${prefix}_VERSION}")
		if ("${${prefix}_Branch}" STREQUAL "master")
			if (NOT "${${prefix}_COMMITS}" STREQUAL "" OR NOT "${${prefix}_HASH}" STREQUAL "")
				message (AUTHOR_WARNING "Commit without a version tag found on branch master for ${prefix}; this indicates a tagging/branching/push error.")
			endif ()
		endif ()
	else ()
		# Try to fetch version through VERSION file
		get_version_from_file(${prefix}_VERSION "${dir}/VERSION")
		if (${${prefix}_VERSION-NOTFOUND})
			message (FATAL_ERROR "Failed to fetch ${prefix} version.")
		else ()
			message (STATUS "${prefix} version fetched from VERSION file: ${${prefix}_VERSION}")
		endif ()
	endif ()

	if (DEFINED ENV{CI})
		message (STATUS "Build on travis-ci detected, not checking version (git clone --depth=...)")
	else ()
		if (NOT "${${prefix}_VERSION}" MATCHES "^${VERSION_REGEX_ANY}$")
			message (FATAL_ERROR "Invalid version format: ${${prefix}_VERSION}")
		endif ()
	endif ()
endmacro ()

macro (TestVersion)
	foreach(version ${VERSION_REGEX_ANY_MATCH_EXAMPLES})
		if (NOT "${version}" MATCHES "^${VERSION_REGEX_ANY}$")
			message (STATUS "^${VERSION_REGEX_ANY}$")
			message (FATAL_ERROR "Invalid version format: ${version}")
		endif ()
	endforeach ()
endmacro ()

#TestVersion()
