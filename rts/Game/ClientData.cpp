/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <sstream>

#include "ClientData.h"
#include "GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/Platform/Misc.h"
#include "System/StringUtil.h"

std::vector<std::uint8_t> ClientData::GetCompressed()
{
	std::ostringstream clientDataStream;

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
	clientDataStream << Platform::GetOSVersionStr() << std::endl;
	clientDataStream << Platform::GetWordSizeStr() << std::endl;

	const std::string& clientData = clientDataStream.str();

	return std::move(zlib::deflate(reinterpret_cast<const std::uint8_t*>(clientData.data()), clientData.size()));
}

std::string ClientData::GetUncompressed(const std::vector<std::uint8_t>& compressed)
{
	std::vector<std::uint8_t> buffer{zlib::inflate(compressed)};
	std::string cdata{buffer.begin(), buffer.end()};

	return (std::move(cdata));
}

