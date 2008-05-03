#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <string>

namespace netcode {
	class RawPacket;
}

class GameData
{
public:
	GameData();
	GameData(const netcode::RawPacket& packet);
	
	const netcode::RawPacket* Pack() const;
	
	void SetScript(const std::string& newScript);
	void SetMap(const std::string& newMap, const unsigned checksum);
	void SetMod(const std::string& newMod, const unsigned checksum);
	void SetRandomSeed(const unsigned seed);
	
	const std::string& GetScript() const {return script;};
	const std::string& GetMap() const {return map;};
	unsigned GetMapChecksum() const {return mapChecksum;};
	const std::string& GetMod() const {return mod;};
	unsigned GetModChecksum() const {return modChecksum;};
	unsigned GetRandomSeed() const {return randomSeed;};

private:
	std::string script;
	std::string map;
	unsigned mapChecksum;
	std::string mod;
	unsigned modChecksum;
	unsigned randomSeed;
};

#endif
