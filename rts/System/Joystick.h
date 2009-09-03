#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <SDL_joystick.h>

void InitJoystick();

class Joystick
{
public:
	Joystick();
	~Joystick();
	
private:
	SDL_Joystick* myStick;
};


extern Joystick* stick;

#endif