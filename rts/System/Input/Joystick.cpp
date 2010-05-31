/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Joystick.h"

#include <SDL.h>

#include "InputHandler.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "EventHandler.h"

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
		inputCon = input.AddHandler(boost::bind(&Joystick::HandleEvent, this, _1));
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

bool Joystick::HandleEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_JOYAXISMOTION:
		{
			eventHandler.JoystickEvent("JoyAxis", event.jaxis.axis, event.jaxis.value);
			break;
		}
		case SDL_JOYHATMOTION:
		{
			eventHandler.JoystickEvent("JoyHat", event.jhat.hat, event.jhat.value);
			break;
		}
		case SDL_JOYBUTTONDOWN:
		{
			eventHandler.JoystickEvent("JoyButtonDown", event.jbutton.button, event.jbutton.state);
			break;
		}
		case SDL_JOYBUTTONUP:
		{
			eventHandler.JoystickEvent("JoyButtonUp", event.jbutton.button, event.jbutton.state);
			break;
		}
		default:
		{
		}
	}
	return false;
}
