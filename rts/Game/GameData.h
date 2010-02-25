/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

namespace netcode {
	class RawPacket;
}

class GameData
{
public:
	GameData();
	GameData(boost::shared_ptr<const netcode::RawPacket>);
	
	const netcode::RawPacket* Pack() const;
	
	void SetSetup(const std::string& newSetup);
	void SetMapChecksum(const unsigned checksum);
	void SetModChecksum(const unsigned checksum);
	void SetRandomSeed(const unsigned seed);
	
	const std::string& GetSetup() const {return setupText;};
	unsigned GetMapChecksum() const {return mapChecksum;};
	unsigned GetModChecksum() const {return modChecksum;};
	unsigned GetRandomSeed() const {return randomSeed;};

private:
	std::string setupText;
	mutable std::vector<boost::uint8_t> compressed;

	unsigned mapChecksum;
	unsigned modChecksum;
	unsigned randomSeed;
};

#endif
