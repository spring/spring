#ifndef GAMEVERSION_H
#define GAMEVERSION_H

#include <string>

namespace SpringVersion
{
	/// major revision number (e.g. 0.77)
	extern const char* const Major;
	
	/// minor revision / bugfix which breaks sync between clients
	extern const char* const Minor;
	
	/** @brief bugfix which keeps sync between clients using the same MAJOR.MINOR version
	
	Client with same Major.Minor can still play together. Also demos should be compatible between patchsets.
	*/
	extern const char* const Patchset;
	
	/// additional information (compiler flags, svn revision etc.)
	extern const char* const Additional;
	
	/// time of build
	extern const char* const BuildTime;
	
	/// Major.Minor
	extern std::string Get();
	
	/// Major.Minor.Patchset (Additional)
	extern std::string GetFull();
};	

#endif
