/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommonDefHandler.h"

#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/ISound.h"
#include "System/Log/ILog.h"

#include <array>

static const std::array<std::string, 2> soundExts = {"wav", "ogg"};

int CommonDefHandler::LoadSoundFile(const std::string& fileName)
{
	const std::string soundExt = std::move(FileSystem::GetExtension(fileName));

	// unlike constructing a CFileHandler this does not read the data
	// into memory; faster for large files and many small individually
	// compressed sounds (e.g. in pool archives)
	const bool foundExt = (std::find(soundExts.begin(), soundExts.end(), soundExt) != soundExts.end());
	const bool haveFile = (foundExt && CFileHandler::FileExists(fileName, SPRING_VFS_RAW_FIRST));
	const bool haveItem = (haveFile || sound->HasSoundItem(fileName));

	if (haveItem)
		return (sound->GetSoundId(fileName));

	const std::string soundFile = "sounds/" + fileName + ((soundExt.empty())? ".wav": "");

	if (CFileHandler::FileExists(soundFile, SPRING_VFS_RAW_FIRST))
		return (sound->GetSoundId(soundFile));

	LOG_L(L_WARNING, "[%s] could not load sound \"%s\" from {Unit,Weapon}Def", __func__, fileName.c_str());
	return 0;
}
