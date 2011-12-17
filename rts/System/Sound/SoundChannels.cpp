/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IAudioChannel.h"
#include "SoundChannels.h"

namespace Channels
{
	std::set<AudioChannelImpl *> All;
	AudioChannelImpl BGMusic;
	AudioChannelImpl General;
	AudioChannelImpl Battle;
	AudioChannelImpl UnitReply;
	AudioChannelImpl UserInterface;
}
