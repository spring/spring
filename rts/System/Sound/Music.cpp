#include "Music.h"

#include "Sound.h"
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

void MusicChannel::Play(const std::string& path, float _volume)
{
	if (!enabled)
	{
		return;
	}

	if (!current)
		current = sound->GetNextBestSource();

	current->PlayStream(path, volume * _volume);
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

unsigned int MusicChannel::GetTime()
{
	if (current)
		return current->GetStreamTime();
	else
		return 0;
}

unsigned int MusicChannel::GetPlayTime()
{
	if (current)
		return current->GetStreamPlayTime();
	else
		return 0;
}


namespace Channels
{
	MusicChannel BGMusic;
}
