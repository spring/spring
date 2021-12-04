/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <string>
#include <vector>
#include <cinttypes>
#include <memory>

namespace netcode {
	class RawPacket;
}

class GameData
{
public:
	GameData();
	GameData(const std::string& setup);
	GameData(std::shared_ptr<const netcode::RawPacket> pckt);

	const netcode::RawPacket* Pack() const;

	void SetSetupText(const std::string& newSetup);
	void SetMapChecksum(const unsigned checksum);
	void SetModChecksum(const unsigned checksum);
	void SetRandomSeed(const unsigned seed);

	const std::string& GetSetupText() const { return setupText; }
	unsigned GetMapChecksum() const { return mapChecksum; }
	unsigned GetModChecksum() const { return modChecksum; }
	unsigned GetRandomSeed() const { return randomSeed; }

private:
	// same as GameSetup::gameSetupText, but after any
	// possible modifications by PreGame or GameServer
	std::string setupText;

	mutable std::vector<std::uint8_t> compressed;

	unsigned mapChecksum;
	unsigned modChecksum;
	unsigned randomSeed;
};

#endif // GAME_DATA_H
