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
	void SetMapChecksum(const uint8_t* checksum) { std::copy(checksum, checksum + sizeof(mapChecksum), mapChecksum); }
	void SetModChecksum(const uint8_t* checksum) { std::copy(checksum, checksum + sizeof(modChecksum), modChecksum); }
	void SetRandomSeed(const uint32_t seed) { randomSeed = seed; }

	const std::string& GetSetupText() const { return setupText; }
	const uint8_t* GetMapChecksum() const { return &mapChecksum[0]; }
	const uint8_t* GetModChecksum() const { return &modChecksum[0]; }
	uint32_t GetRandomSeed() const { return randomSeed; }

private:
	// same as GameSetup::gameSetupText, but after any
	// possible modifications by PreGame or GameServer
	std::string setupText;

	mutable std::vector<std::uint8_t> compressed;

	uint8_t mapChecksum[64];
	uint8_t modChecksum[64];

	uint32_t randomSeed = 0;
};

#endif // GAME_DATA_H
