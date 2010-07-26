/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <boost/format.hpp>
#include <assert.h>
#include <zlib.h>

#include "mmgr.h"

#include "GameData.h"

#include "BaseNetProtocol.h"
#include "Net/PackPacket.h"
#include "Net/UnpackPacket.h"

using namespace netcode;

GameData::GameData()
{
	mapChecksum = 0;
	modChecksum = 0;
	randomSeed = 0;
}

GameData::GameData(boost::shared_ptr<const RawPacket> pckt)
{
	assert(pckt->data[0] == NETMSG_GAMEDATA);

	UnpackPacket packet(pckt, 3);
	boost::uint16_t compressedSize;
	packet >> compressedSize;
	compressed.resize(compressedSize);
	packet >> compressed;

	// "the LSB does not describe any mechanism by which a
	// compressor can communicate the size required to the
	// uncompressor" ==> we must reserve some fixed-length
	// buffer (256K bytes to handle very large scripts)
	static const unsigned long bufSize = 262144;
	unsigned long rawSize = bufSize;

	std::vector<boost::uint8_t> buffer(bufSize);

	if (uncompress(&buffer[0], &rawSize, &compressed[0], compressed.size()) != Z_OK) {
		static const std::string err = str(boost::format("unpacking game-data packet failed (%u bytes decompressed)") %rawSize);

		throw UnpackPacketException(err.c_str());
	}

	setupText = reinterpret_cast<char*>(&buffer[0]);

	packet >> mapChecksum;
	packet >> modChecksum;
	packet >> randomSeed;
}

const netcode::RawPacket* GameData::Pack() const
{
	if (compressed.empty())
	{
		long unsigned bufsize = setupText.size()*1.02+32;
		compressed.resize(bufsize);
		const int error = compress(&compressed[0], &bufsize, reinterpret_cast<const boost::uint8_t*>(setupText.c_str()), setupText.length());
		compressed.resize(bufsize);
		assert(error == Z_OK);
	}

	unsigned short size = 3 + 2*sizeof(unsigned) + compressed.size()+2 + 4;
	PackPacket* buffer = new PackPacket(size, NETMSG_GAMEDATA);
	*buffer << size;
	*buffer << boost::uint16_t(compressed.size());
	*buffer << compressed;
	*buffer << mapChecksum;
	*buffer << modChecksum;
	*buffer << randomSeed;
	return buffer;
}

void GameData::SetSetup(const std::string& newSetup)
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
