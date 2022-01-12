/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "GameData.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Net/UnpackPacket.h"
#include "System/StringUtil.h"

using namespace netcode;

GameData::GameData()
{
	std::memset(mapChecksum, 0, sizeof(mapChecksum));
	std::memset(modChecksum, 0, sizeof(modChecksum));
}
GameData::GameData(const std::string& setup): setupText(setup)
{
	std::memset(mapChecksum, 0, sizeof(mapChecksum));
	std::memset(modChecksum, 0, sizeof(modChecksum));
}

GameData::GameData(std::shared_ptr<const RawPacket> pckt)
{
	assert(pckt->data[0] == NETMSG_GAMEDATA);

	UnpackPacket packet(pckt, 3);

	std::uint16_t compressedSize;

	packet >> compressedSize; compressed.resize(compressedSize);
	packet >> compressed;

	const std::vector<std::uint8_t> buffer{zlib::inflate(compressed)};

	if (buffer.empty())
		throw netcode::UnpackPacketException("Error decompressing GameData");

	// avoid reinterpret_cast; buffer is not null-terminated
	setupText = {buffer.begin(), buffer.end()};

	for (auto& checksum: mapChecksum) { packet >> checksum; }
	for (auto& checksum: modChecksum) { packet >> checksum; }

	packet >> randomSeed;
}

const netcode::RawPacket* GameData::Pack() const
{
	if (compressed.empty())
		compressed = std::move(zlib::deflate(reinterpret_cast<const std::uint8_t*>(setupText.data()), setupText.size()));

	assert(!compressed.empty());
	assert(compressed.size() <= std::numeric_limits<std::uint16_t>::max());

	const std::uint16_t size = 3 + sizeof(mapChecksum) + sizeof(modChecksum) + compressed.size() + 2 + sizeof(uint32_t);
	PackPacket* buffer = new PackPacket(size, NETMSG_GAMEDATA);
	*buffer << size;
	*buffer << std::uint16_t(compressed.size());
	*buffer << compressed;

	for (auto& checksum: mapChecksum) { *buffer << checksum; }
	for (auto& checksum: modChecksum) { *buffer << checksum; }

	*buffer << randomSeed;
	return buffer;
}

void GameData::SetSetupText(const std::string& newSetup)
{
	setupText = newSetup;
	compressed.clear();
}

