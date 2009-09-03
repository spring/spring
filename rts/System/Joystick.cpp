#include "Joystick.h"

#include <SDL.h>

#include "ConfigHandler.h"
#include "LogOutput.h"

Joystick* stick = NULL;

void InitJoystick()
{
	const bool useJoystick = configHandler->Get("JoystickEnabled", true);
	if (useJoystick)
	{
		const int err = SDL_InitSubSystem(SDL_INIT_JOYSTICK);
		if (err == -1)
		{
			LogObject() << "Could not initialise joystick subsystem: " << SDL_GetError();
			return;
		}
		else
		{
			stick = new Joystick();
		}
	};
}

Joystick::Joystick()
{
	const int numSticks = SDL_NumJoysticks();
	LogObject() << "Joysticks found: " << numSticks;
	
	const int stickNum = configHandler->Get("JoystickUse", 0);
	myStick =  SDL_JoystickOpen(stickNum);
	
	if (myStick)
	{
		LogObject() << "Using joystick " << stickNum << ": " << SDL_JoystickName(stickNum);
	}
	else
	{
		LogObject() << "Joystick " << stickNum << " not found";
	}
}

Joystick::~Joystick()
{
	if (myStick)
		SDL_JoystickClose(myStick);
}
