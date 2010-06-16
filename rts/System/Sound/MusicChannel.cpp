/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MusicChannel.h"

#include "ISound.h"
#include "SoundSource.h"

MusicChannel::MusicChannel() : current(NULL)
{
}

MusicChannel::~MusicChannel()
{
}

void MusicChannel::SetVolume(float newvolume)
{
	volume = newvolume;

	if (current)
		current->SetVolume(volume); // OpenAL is threadsafe enought so no lock needed
}

void MusicChannel::Enable(bool newState)
{
	enabled = newState;

	if (!enabled)
	{
		Stop();
	}
}

void MusicChannel::Play(const std::string& path, float _volume, bool enqueue)
{
	if (!enabled)
	{
		return;
	}

	if (!current)
		current = sound->GetNextBestSource(); // may return 0 if no sources available

	if (current)
		current->PlayStream(path, volume * _volume, enqueue);
}

void MusicChannel::Pause()
{
	if (current)
		current->StreamPause();
}

void MusicChannel::Stop()
{
	if (current)
	{
		current->StreamStop();
		current = NULL;
	}
}

float MusicChannel::GetTime()
{
	if (current)
		return current->GetStreamTime();
	else
		return 0;
}

float MusicChannel::GetPlayTime()
{
	if (current)
		return current->GetStreamPlayTime();
	else
		return 0;
}
