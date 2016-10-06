/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InputHandler.h"
#include "Joystick.h"

#include <SDL.h>
#include <functional>

#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/EventHandler.h"

CONFIG(bool, JoystickEnabled).defaultValue(true).headlessValue(false);
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
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

Joystick::Joystick()
	: myStick(nullptr)
{
	const int numSticks = SDL_NumJoysticks();
	LOG("Joysticks found: %i", numSticks);
	if (numSticks <= 0)
		return;

	const int stickNum = configHandler->GetInt("JoystickUse");

	myStick = SDL_JoystickOpen(stickNum);

	if (myStick)
	{
		LOG("Using joystick %i: %s", stickNum, SDL_JoystickName(myStick));
		inputCon = input.AddHandler(std::bind(&Joystick::HandleEvent, this, std::placeholders::_1));
	}
	else
	{
		LOG_L(L_ERROR, "Joystick %i not found", stickNum);
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
		case SDL_JOYDEVICEADDED: //TODO
			LOG("Joystick has been added");
			break;
		case SDL_JOYDEVICEREMOVED:
			LOG("Joystick has been removed");
			break;
		default:
		{
		}
	}
	return false;
}
