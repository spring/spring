/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_SOUND_SET_H
#define GUI_SOUND_SET_H

#include <algorithm>
#include <cstdlib>
#include <string>

#include "Sim/Misc/CommonDefHandler.h"

struct GuiSoundSetData {
	GuiSoundSetData() = default;
	GuiSoundSetData(const std::string& n, int i, float v): name(n), id(i), volume(v) {}

	std::string name;
	int id = -1;
	float volume = 0.0f;
};

struct GuiSoundSet {
public:
public:
	const GuiSoundSetData& GetSoundData(int relDataIdx) const {
		if (!ValidIndex(minSoundDataIdx + relDataIdx))
			return CommonDefHandler::GetSoundSetData(0);

		return CommonDefHandler::GetSoundSetData(minSoundDataIdx + relDataIdx);
	}


	/// get a (loaded) sound's ID for set-relative index \<relDataIdx\>
	int getID(int relDataIdx) const {
		if (!ValidIndex(minSoundDataIdx + relDataIdx))
			return 0;

		GuiSoundSetData& soundSet = CommonDefHandler::GetSoundSetData(minSoundDataIdx + relDataIdx);

		if (soundSet.id == -1)
			soundSet.id = CommonDefHandler::LoadSoundFile(soundSet.name);

		return soundSet.id;
	}

	/// get a (loaded) sound's volume for set-relative index \<relDataIdx\>
	float getVolume(int relDataIdx) const { return (GetSoundData(relDataIdx).volume); }

	/// set a (loaded) sound's volume for set-relative index \<relDataIdx\>
	void setVolume(int relDataIdx, float volume) {
		const_cast<GuiSoundSetData&>(GetSoundData(relDataIdx)).volume = volume;
	}

	void UpdateIndices(size_t maxDataIdx) {
		minSoundDataIdx = std::min(minSoundDataIdx, maxDataIdx);
		maxSoundDataIdx = maxDataIdx;
	}

	size_t NumSounds() const {
		return ((maxSoundDataIdx + 1 - minSoundDataIdx) * ValidIndex(minSoundDataIdx + 0));
	}

private:
	bool ValidIndex(size_t absDataIdx) const { return (absDataIdx >= minSoundDataIdx && absDataIdx <= maxSoundDataIdx); }

private:
	// data indices into CommonDefHandler::soundSetData comprising this set
	size_t minSoundDataIdx = 1u << 31u;
	size_t maxSoundDataIdx = 0;
};

#endif // GUI_SOUND_SET

