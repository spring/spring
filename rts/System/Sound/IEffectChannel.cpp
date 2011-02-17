/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IEffectChannel.h"

IEffectChannel::IEffectChannel()
	: emmitsPerFrame(32)
	, emmitsThisFrame(0)
{
}

namespace Channels
{
	EffectChannelImpl General;
	EffectChannelImpl Battle;
	EffectChannelImpl UnitReply;
	EffectChannelImpl UserInterface;
}
