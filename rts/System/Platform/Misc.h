#ifndef PLATFORM_MISC_H
#define PLATFORM_MISC_H

#include <string>

namespace Platform
{
/**
 * Returns the path to this binary, excluding the file name.
 * examples:
 * /usr/games/
 * C:\Program Files\Spring\
 */
std::string GetBinaryPath();

/**
 * Returns the path to the spring synchronization library (unitsync),
 * excluding the file name.
 * examples:
 * /usr/lib/
 * C:\Program Files\Spring\
 */
std::string GetLibraryPath();

/**
 * Returns the path to to this binary, filename inclusive.
 * examples:
 * /usr/games/spring
 * C:\Program Files\Spring\spring.exe
 */
std::string GetBinaryFile();

std::string GetOS();
bool Is64Bit();
bool Is32BitEmulation();
}

#endif // PLATFORM_MISC_H
