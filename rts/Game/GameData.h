#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <string>

#include "Net/UnpackPacket.h"
namespace netcode {
	class RawPacket;
}

class GameData
{
public:
	GameData();
	GameData(netcode::UnpackPacket* packet);
	
	const netcode::RawPacket* Pack() const;
	
	void SetScript(const std::string& newScript);
	void SetMap(const std::string& newMap, const unsigned checksum);
	void SetMod(const std::string& newMod, const unsigned checksum);
	
	const std::string& GetScript() const {return script;};
	const std::string& GetMap() const {return map;};
	unsigned GetMapChecksum() const {return mapChecksum;};
	const std::string& GetMod() const {return mod;};
	unsigned GetModChecksum() const {return modChecksum;};

private:
	std::string script;
	std::string map;
	unsigned mapChecksum;
	std::string mod;
	unsigned modChecksum;
};

#endif
