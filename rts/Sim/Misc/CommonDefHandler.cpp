/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommonDefHandler.h"

#include "FileSystem/FileSystem.h"
#include "FileSystem/FileHandler.h"
#include "Sound/ISound.h"
#include "LogOutput.h"

int CommonDefHandler::LoadSoundFile(const std::string& fileName)
{
	const std::string extension = filesystem.GetExtension(fileName);
	bool hasFile = false;
	if (extension == "wav" || extension == "ogg")
	{
		CFileHandler raw(fileName);
		if (raw.FileExists())
			hasFile = true;
	}

	if (!sound->HasSoundItem(fileName) && !hasFile)
	{
		std::string soundFile = "sounds/" + fileName;

		if (extension.empty()) {
			// extension missing, try wav
			soundFile += ".wav";
		}
		CFileHandler fh(soundFile);
		if (fh.FileExists()) {
			// we have a valid soundfile: store name, ID, and default volume
			const int id = sound->GetSoundId(soundFile);
			return id;
		}
		else
		{
			LogObject() << "Could not load sound from def: " << fileName;
			return 0;
		}
	}
	else
	{
		const int id = sound->GetSoundId(fileName);
		return id;
	}
}
