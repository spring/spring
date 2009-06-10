#include "Music.h"

#include <boost/thread/mutex.hpp>

#include "OggStream.h"

namespace music
{

struct TrackItem
{
	std::string path;
	float volume;
};

volatile bool playing = false;

COggStream oggStream; // not threadsafe, only used from musicThread

boost::mutex musicMutex;
TrackItem nextTrack; // protected by musicMutex
volatile bool playNext = false; // protected by musicMutex

void UpdateMusicStream()
{
	boost::mutex::scoped_lock updaterLock(musicMutex);
	if (playing)
	{
		// update buffers
		if (playNext)
		{
			oggStream.Stop(); // just to be sure
			oggStream.Play(nextTrack.path, nextTrack.volume);
			playNext = false;
		}
		oggStream.Update();
	}
	else
	{
		oggStream.Stop();
	}
};

void Play(const std::string& path, float volume)
{
	if (!playing)
	{
		// no thread running, no lock needed
		playing = true;
		playNext = true;
		nextTrack.path = path;
		nextTrack.volume = volume;
	}
	else
	{
		boost::mutex::scoped_lock updaterLock(musicMutex);
		playNext = true;
		nextTrack.path = path;
		nextTrack.volume = volume;
	}
}

void Stop()
{
	playing = false;
}

void Pause()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	oggStream.TogglePause();
}

unsigned int GetTime()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	return oggStream.GetTotalTime();
}

unsigned int GetPlayTime()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	return oggStream.GetPlayTime();
}

void SetVolume(float v)
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	oggStream.SetVolume(v);
}

}
