#include "Music.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "OggStream.h"

namespace music
{

struct TrackItem
{
	std::string path;
	float volume;
};

boost::thread* musicThread = NULL;
volatile bool playing = false;

COggStream oggStream; // not threadsafe, only used from musicThread

boost::mutex musicMutex;
TrackItem nextTrack; // protected by musicMutex
volatile bool playNext = false; // protected by musicMutex

void UpdateMusicStream()
{
	while (playing)
	{
		{ // sleep some time
			boost::xtime xt;
			boost::xtime_get(&xt,boost::TIME_UTC);
			xt.nsec += 200;
			boost::this_thread::sleep(xt);
		}

		{ // update buffers
			boost::mutex::scoped_lock updaterLock(musicMutex);
			if (playNext)
			{
				oggStream.Stop(); // just to be sure
				oggStream.Play(nextTrack.path, nextTrack.volume);
				playNext = false;
			}
			oggStream.Update();
		}
	}
	
	{
		boost::mutex::scoped_lock updaterLock(musicMutex);
		oggStream.Stop();
	}
};

void Play(const std::string& path, float volume)
{
	if (!playing)
	{
		if (musicThread)
		{
			musicThread->join();
			delete musicThread;
		}
		// no thread running, no lock needed
		playing = true;
		playNext = true;
		nextTrack.path = path;
		nextTrack.volume = volume;
		musicThread = new boost::thread(UpdateMusicStream);
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
