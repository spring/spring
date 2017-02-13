/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <zlib.h>

#include "GameData.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Net/UnpackPacket.h"

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
	packet >> compressedSize;
	compressed.resize(compressedSize);
	packet >> compressed;

	// "the LSB does not describe any mechanism by which a
	// compressor can communicate the size required to the
	// uncompressor" ==> we must reserve some fixed-length
	// buffer (starting at 256K bytes to handle very large
	// scripts) for each new decompression attempt
	unsigned long bufSize = 256 * 1024;
	unsigned long rawSize = bufSize;

	std::vector<std::uint8_t> buffer(bufSize);

	int ret;
	while ((ret = uncompress(&buffer[0], &rawSize, &compressed[0], compressed.size())) == Z_BUF_ERROR) {
		bufSize *= 2;
		rawSize  = bufSize;

		buffer.resize(bufSize);
	}
	if (ret != Z_OK)
		throw netcode::UnpackPacketException("Error while decompressing GameData");

	setupText = reinterpret_cast<char*>(&buffer[0]);

	packet >> mapChecksum;
	packet >> modChecksum;
	packet >> randomSeed;
}

const netcode::RawPacket* GameData::Pack() const
{
	if (compressed.empty()) {
		long unsigned bufsize = compressBound(setupText.size());
		compressed.resize(bufsize);
		const int error = compress(&compressed[0], &bufsize, reinterpret_cast<const std::uint8_t*>(setupText.c_str()), setupText.length());
		compressed.resize(bufsize);
		assert(error == Z_OK);
	}

	unsigned short size = 3 + 2*sizeof(unsigned) + compressed.size()+2 + 4;
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
