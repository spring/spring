#include "Music.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "OggStream.h"

namespace music
{

// Ogg-Vorbis audio stream object
COggStream oggStream;

boost::thread* musicThread = NULL;
boost::mutex musicMutex;
volatile bool playing = false;

void UpdateMusicStream()
{
	playing = true;
	while (playing)
	{
		{ // sleep a second
			boost::xtime xt;
			boost::xtime_get(&xt,boost::TIME_UTC);
			xt.nsec += 200;
			musicThread->sleep(xt);
		}

		{ // update buffers
			boost::mutex::scoped_lock updaterLock(musicMutex);
			oggStream.Update();
		}
	}
	
	oggStream.Stop();
};

void Play(const std::string& path, float volume)
{
	boost::mutex::scoped_lock controlLock(musicMutex);
	oggStream.Play(path, volume);
	if (!playing)
	{
		musicThread = new boost::thread(UpdateMusicStream);
	}
}

void Stop()
{
	boost::mutex::scoped_lock controlLock(musicMutex);
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
