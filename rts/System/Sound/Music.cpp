#include "Music.h"


#include "OggStream.h"

MusicChannel::MusicChannel() : playNext(false), playing(false)
{
}

MusicChannel::~MusicChannel()
{
}

void MusicChannel::SetVolume(float newvolume)
{
	volume = newvolume;
	boost::mutex::scoped_lock controlLock(musicMutex);
	oggStream.SetVolume(nextTrack.volume * volume);
}

void MusicChannel::Enable(bool newState)
{
	enabled = newState;
	playing = false;
}

void MusicChannel::Play(const std::string& path, float volume)
{
	if (!enabled)
	{
		return;
	}

	playNext = true;
	nextTrack.path = path;
	nextTrack.volume = volume;
}

void MusicChannel::Pause()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	oggStream.TogglePause();
}

void MusicChannel::Stop()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	playing = false;
}

unsigned int MusicChannel::GetTime()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	return oggStream.GetTotalTime();
}

unsigned int MusicChannel::GetPlayTime()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	return oggStream.GetPlayTime();
}

void MusicChannel::Update()
{
	if (playing || playNext)
	{
		boost::mutex::scoped_lock updaterLock(musicMutex);
		// update buffers
		if (playNext)
		{
			oggStream.Stop(); // just to be sure
			oggStream.Play(nextTrack.path, nextTrack.volume * volume);
			playNext = false;
		}
		oggStream.Update();
	}
};


namespace Channels
{
	MusicChannel BGMusic;
}
