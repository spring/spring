/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommonDefHandler.h"

#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/ISound.h"
#include "System/Log/ILog.h"

int CommonDefHandler::LoadSoundFile(const std::string& fileName)
{
	const std::string extension = std::move(FileSystem::GetExtension(fileName));
	bool hasFile = false;

	#if 0
	if (extension == "wav" || extension == "ogg") {
		CFileHandler raw(fileName);
		hasFile = raw.FileExists();
	}
	#else
	// this does not read the data into memory; faster for large
	// files and many small individually compressed sounds (e.g.
	// in pool archives)
	if (extension == "wav" || extension == "ogg")
		hasFile = CFileHandler::FileExists(fileName, SPRING_VFS_RAW_FIRST);
	#endif

	if (!hasFile && !sound->HasSoundItem(fileName)) {
		// try wav if extension is missing
		const std::string soundFile = "sounds/" + fileName + ((extension.empty())? ".wav": "");

		#if 0
		CFileHandler fh(soundFile);

		// we have a valid soundfile: store name, ID, and default volume
		if (fh.FileExists())
			return (sound->GetSoundId(soundFile));
		#else
		if (CFileHandler::FileExists(soundFile, SPRING_VFS_RAW_FIRST))
			return (sound->GetSoundId(soundFile));
		#endif

		LOG_L(L_WARNING, "[%s] could not load sound \"%s\" from {Unit,Weapon}Def", __func__, fileName.c_str());
		return 0;
	}

	return (sound->GetSoundId(fileName));
}
