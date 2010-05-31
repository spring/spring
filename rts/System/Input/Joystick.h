/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <SDL_joystick.h>
#include <boost/signals/connection.hpp>

union SDL_Event;

void InitJoystick();

class Joystick
{
public:
	Joystick();
	~Joystick();
	
private:
	bool HandleEvent(const SDL_Event& event);
	boost::signals::scoped_connection inputCon;

	SDL_Joystick* myStick;
};


extern Joystick* stick;

#endif
