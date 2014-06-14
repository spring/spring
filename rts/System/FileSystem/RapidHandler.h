/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * searches for the name of an archive by the rapid tag in <datadirs>/rapid/.../versions.gz
 * @param tag of the rapid tag to search for, for example ba:stable
 * @return name of the rapid tag, empty string if not found, for example "Balanced Annihilation v8.00"
 */
std::string GetRapidName(const std::string& tag);
