#include "ModSound.h"


#include "Lua/LuaParser.h"
#include "Sound.h"
#include "LogOutput.h"
#include "Util.h"

ModSound& ModSound::Get()
{
	static ModSound instance;
	return instance;
}

std::string ModSound::GetSoundFile(const std::string& name)
{
	SoundMap::const_iterator it = sounds.find(name);
	if (it != sounds.end())
		return it->second;
	else
		return std::string("");
}

unsigned ModSound::GetSoundId(const std::string& name)
{
	SoundMap::const_iterator it = sounds.find(StringToLower(name));
	if (it != sounds.end())
		return sound->GetWaveId(it->second);
	else
		return 0;
}

ModSound::ModSound()
{
	LuaParser parser("gamedata/sounds.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!parser.IsValid()) {
		logOutput.Print("Sounds.lua error:");
		logOutput.Print(parser.GetErrorLog());
	}
	parser.Execute();
	const LuaTable root = parser.GetRoot();
	const LuaTable Sounds = root.SubTable("engineEvents");
	Sounds.GetMap(sounds);
	assert(!sounds.empty());
}