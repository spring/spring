/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H

#include <cinttypes>
#include <string>
#include <vector>


namespace ClientData {
	std::vector<std::uint8_t> GetCompressed();
	std::string GetUncompressed(const std::vector<std::uint8_t>& compressed);
}


#endif // CLIENT_DATA_H
