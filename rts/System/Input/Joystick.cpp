/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InputHandler.h"
#include "Joystick.h"

#include <SDL.h>

#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/EventHandler.h"

CONFIG(bool, JoystickEnabled).defaultValue(true);
CONFIG(int, JoystickUse).defaultValue(0);

Joystick* stick = NULL;

void InitJoystick()
{
	const bool useJoystick = configHandler->GetBool("JoystickEnabled");
	if (useJoystick)
	{
		const int err = SDL_InitSubSystem(SDL_INIT_JOYSTICK);
		if (err == -1)
		{
			LOG_L(L_ERROR, "Could not initialise joystick subsystem: %s",
					SDL_GetError());
			return;
		}
		else
		{
			stick = new Joystick();
		}
	};
}

void FreeJoystick() {
	if (stick != NULL) {
		delete stick;
		stick = NULL;
	}
}

Joystick::Joystick()
{
	const int numSticks = SDL_NumJoysticks();
	LOG("Joysticks found: %i", numSticks);

	const int stickNum = configHandler->GetInt("JoystickUse");

	myStick =  SDL_JoystickOpen(stickNum);

	if (myStick)
	{
		LOG("Using joystick %i: %s", stickNum, SDL_JoystickName(stickNum));
		inputCon = input.AddHandler(boost::bind(&Joystick::HandleEvent, this, _1));
	}
	else
	{
		LOG_L(L_WARNING, "Joystick %i not found", stickNum);
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
