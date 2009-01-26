#ifndef MODSOUNDS_H
#define MODSOUNDS_H

#include <map>
#include <string>

class ModSound
{
public:
	static ModSound& Get();
	std::string GetSoundFile(const std::string& name);
	unsigned GetSoundId(const std::string& name);

private:
	ModSound();
	typedef std::map<std::string, std::string> SoundMap;
	SoundMap sounds;
};

#endif