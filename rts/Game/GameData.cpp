/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "GameData.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Net/UnpackPacket.h"
#include "System/Util.h"

using namespace netcode;

GameData::GameData()
	: mapChecksum(0)
	, modChecksum(0)
	, randomSeed(0)
{
}


GameData::GameData(const std::string& setup)
	: setupText(setup)
	, mapChecksum(0)
	, modChecksum(0)
	, randomSeed(0)
{
}

GameData::GameData(std::shared_ptr<const RawPacket> pckt)
{
	assert(pckt->data[0] == NETMSG_GAMEDATA);

	UnpackPacket packet(pckt, 3);

	std::uint16_t compressedSize;

	packet >> compressedSize; compressed.resize(compressedSize);
	packet >> compressed;

	std::vector<std::uint8_t> buffer{zlib::inflate(compressed)};

	if (buffer.empty())
		throw netcode::UnpackPacketException("Error decompressing GameData");

	setupText = reinterpret_cast<char*>(&buffer[0]);

	packet >> mapChecksum;
	packet >> modChecksum;
	packet >> randomSeed;
}

const netcode::RawPacket* GameData::Pack() const
{
	if (compressed.empty())
		compressed = std::move(zlib::deflate(reinterpret_cast<const std::uint8_t*>(setupText.data()), setupText.size()));

	assert(!compressed.empty());
	assert(compressed.size() <= std::numeric_limits<std::uint16_t>::max());

	const std::uint16_t size = 3 + 2 * sizeof(std::uint32_t) + compressed.size() + 2 + 4;
	PackPacket* buffer = new PackPacket(size, NETMSG_GAMEDATA);
	*buffer << size;
	*buffer << std::uint16_t(compressed.size());
	*buffer << compressed;
	*buffer << mapChecksum;
	*buffer << modChecksum;
	*buffer << randomSeed;
	return buffer;
}

void GameData::SetSetupText(const std::string& newSetup)
{
	setupText = newSetup;
	compressed.clear();
}

void GameData::SetMapChecksum(const unsigned checksum)
{
	mapChecksum = checksum;
}

void GameData::SetModChecksum(const unsigned checksum)
{
	modChecksum = checksum;
}

void GameData::SetRandomSeed(const unsigned seed)
{
	randomSeed = seed;
}
