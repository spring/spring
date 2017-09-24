/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_SOUND_SET_H
#define GUI_SOUND_SET_H

#include <cstdlib>
#include <vector>
#include <string>

#include "Sim/Misc/CommonDefHandler.h"

struct GuiSoundSet
{
public:
	struct Data {
		Data() : id(-1), volume(0.0f) {}
		Data(const std::string& n, int i, float v): name(n), id(i), volume(v) {}

		std::string name;
		mutable int id;
		float volume;
	};

public:
	bool ValidIndex(size_t idx) const { return (idx < sounds.size()); }

	const Data& getSound(int idx) const { return sounds[idx]; }
	/// get a (loaded) sound's name for index \<idx\>
	const std::string getName(int idx) const { return ((ValidIndex(idx))? sounds[idx].name : ""); }

	/// get a (loaded) sound's ID for index \<idx\>
	// int getID(int idx) const { return (ValidIndex(idx) ? sounds[idx].id : 0); }
	int getID(int idx) const {
		if (!ValidIndex(idx))
			return 0;
		if (sounds[idx].id == -1)
			sounds[idx].id = CommonDefHandler::LoadSoundFile(sounds[idx].name);

		return sounds[idx].id;
	}

	/// get a (loaded) sound's volume for index \<idx\>
	float getVolume(int idx) const { return ((ValidIndex(idx))? sounds[idx].volume: 0.0f); }


	/// set a (loaded) sound's name for index \<idx\>
	void setName(int idx, std::string name) {
		if (!ValidIndex(idx))
			return;
		sounds[idx].name = name;
	}

	/// set a (loaded) sound's ID for index \<idx\>
	void setID(int idx, int id) {
		if (!ValidIndex(idx))
			return;
		sounds[idx].id = id;
	}

	/// set a (loaded) sound's volume for index \<idx\>
	void setVolume(int idx, float volume) {
		if (!ValidIndex(idx))
			return;
		sounds[idx].volume = volume;
	}

public:
	std::vector<Data> sounds;
};

#endif // GUI_SOUND_SET
