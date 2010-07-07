/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_AUDIO_CHANNEL_H
#define I_AUDIO_CHANNEL_H

/**
 * @brief Channel for playing sounds
 *
 * Has its own volume "slider", and can be enabled / disabled seperately.
 * Abstract base class.
 */
class IAudioChannel {
protected:
	IAudioChannel();

public:
	virtual void Enable(bool newState)
	{
		enabled = newState;
	}
	bool IsEnabled() const
	{
		return enabled;
	}

	virtual void SetVolume(float newVolume)
	{
		volume = newVolume;
	}
	float GetVolume() const
	{
		return volume;
	}

protected:
	float volume;
	bool enabled;
};

#endif // I_AUDIO_CHANNEL_H
