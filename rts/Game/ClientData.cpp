/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <zlib.h>

#include <cassert>
#include <sstream>

#include "ClientData.h"
#include "GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/Platform/Misc.h"

std::vector<std::uint8_t> ClientData::GetCompressed()
{
	std::ostringstream clientDataStream;
	std::vector<std::uint8_t> buf;


	// save user's non-default config; exclude non-engine tags
	for (const auto& it: configHandler->GetDataWithoutDefaults()) {
		if (ConfigVariable::GetMetaData(it.first) == nullptr)
			continue;
		clientDataStream << it.first << " = " << it.second << std::endl;
	}

	clientDataStream << std::endl;

	clientDataStream << SpringVersion::GetFull() << std::endl;
	clientDataStream << SpringVersion::GetBuildEnvironment() << std::endl;
	clientDataStream << SpringVersion::GetCompiler() << std::endl;
	clientDataStream << Platform::GetOS() << std::endl;
	clientDataStream << Platform::GetWordSizeStr() << std::endl;

	const std::string& clientData = clientDataStream.str();

	long unsigned bufsize = compressBound(clientData.size());
	buf.resize(bufsize);
	const int error = compress(&buf[0], &bufsize, reinterpret_cast<const std::uint8_t*>(clientData.c_str()), clientData.size());
	buf.resize(bufsize);
	assert(error == Z_OK);

	return std::move(buf);
}


std::string ClientData::GetUncompressed(const std::vector<std::uint8_t>& compressed)
{
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
		return "";

	return std::move(std::string(buffer.begin(),buffer.begin() + rawSize));
}
