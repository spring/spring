/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <array>

#include "CommonDefHandler.h"

#include "Sim/Misc/GuiSoundSet.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/ISound.h"
#include "System/Log/ILog.h"

static const std::array<std::string, 2> soundExts = {{"wav", "ogg"}};

// UnitDef and WeaponDef sound-sets; [0] is always a dummy
static std::vector<GuiSoundSetData> soundSetData;


void CommonDefHandler::InitStatic()
{
	soundSetData.clear();
	soundSetData.reserve(4096);
	soundSetData.emplace_back();
}

void CommonDefHandler::KillStatic()
{
	LOG_L(L_INFO, "[CommonDefHandler::%s] %u sound-set data items added", __func__, uint32_t(soundSetData.size()));
}


void CommonDefHandler::AddSoundSetData(GuiSoundSet& soundSet, const std::string& fileName, float volume)
{
	// NB: for each set, all data variants should be loaded sequentially
	soundSet.UpdateIndices(soundSetData.size());
	soundSetData.emplace_back(fileName, -1, volume);
}

GuiSoundSetData& CommonDefHandler::GetSoundSetData(size_t idx)
{
	return (soundSetData.at(idx));
}

size_t CommonDefHandler::SoundSetDataCount()
{
	return (soundSetData.size());
}


int CommonDefHandler::LoadSoundFile(const std::string& fileName)
{
	if (!fileName.empty()) {
		const std::string soundExt = std::move(FileSystem::GetExtension(fileName));

		// unlike constructing a CFileHandler this does not read the data
		// into memory; faster for large files and many small individually
		// compressed sounds (e.g. in pool archives)
		const bool foundExt = (std::find(soundExts.cbegin(), soundExts.cend(), soundExt) != soundExts.cend());
		const bool haveFile = (foundExt && CFileHandler::FileExists(fileName, SPRING_VFS_RAW_FIRST));
		const bool haveItem = (haveFile || sound->HasSoundItem(fileName));

		if (haveItem)
			return (sound->GetSoundId(fileName));

		const std::string soundFile = "sounds/" + fileName + ((soundExt.empty())? ".wav": "");

		if (CFileHandler::FileExists(soundFile, SPRING_VFS_RAW_FIRST))
			return (sound->GetSoundId(soundFile));

		LOG_L(L_WARNING, "[%s] could not load sound \"%s\" from {Unit,Weapon}Def", __func__, fileName.c_str());
	}

	return 0;
}
