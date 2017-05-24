/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_SOUND_SET_H
#define GUI_SOUND_SET_H

#include <cstdlib>
#include <vector>
#include <string>

struct GuiSoundSet
{
	struct Data {
		Data() : name(""), id(0), volume(0.0f) {}
		Data(const std::string& n, int i, float v): name(n), id(i), volume(v) {}

		std::string name;
		int id;
		float volume;
	};
	std::vector<Data> sounds;

	bool ValidIndex(int idx) const {
		return ((idx >= 0) && (idx < (int)sounds.size()));
	}

	/// get a (loaded) sound's name for index \<idx\>
	std::string getName(int idx) const {
		return ValidIndex(idx) ? sounds[idx].name : "";
	}

	/// get a (loaded) sound's ID for index \<idx\>
	int getID(int idx) const {
		return ValidIndex(idx) ? sounds[idx].id : 0;
	}

	/// get a (loaded) sound's volume for index \<idx\>
	float getVolume(int idx) const {
		return ValidIndex(idx) ? sounds[idx].volume : 0.0f;
	}

	/// set a (loaded) sound's name for index \<idx\>
	void setName(int idx, std::string name) {
		if (ValidIndex(idx)) {
			sounds[idx].name = name;
		}
	}
	/// set a (loaded) sound's ID for index \<idx\>
	void setID(int idx, int id) {
		if (ValidIndex(idx)) {
			sounds[idx].id = id;
		}
	}
	/// set a (loaded) sound's volume for index \<idx\>
	void setVolume(int idx, float volume) {
		if (ValidIndex(idx)) {
			sounds[idx].volume = volume;
		}
	}
};

#endif // GUI_SOUND_SET
