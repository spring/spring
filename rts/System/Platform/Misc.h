#ifndef PLATFORM_MISC_H
#define PLATFORM_MISC_H

#include <string>

namespace Platform
{
std::string GetBinaryPath();
std::string GetBinaryFile();

std::string GetOS();
bool Is64Bit();
bool Is32BitEmulation();
}

#endif
