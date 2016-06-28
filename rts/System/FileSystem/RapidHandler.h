/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

/*
 * searches for the name of an archive by the rapid tag in <datadirs>/rapid/.../versions.gz
 * @param tag of the rapid tag to search for, for example ba:stable
 * @return name of the rapid tag i.e. "Balanced Annihilation v8.00", "tag" if not found
 */
std::string GetRapidPackageFromTag(const std::string& tag);

std::string GetRapidTagFromPackage(const std::string& pkg);
