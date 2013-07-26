/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <boost/signals2.hpp>
#include <SDL_events.h>

/**
 * @brief Simple thing: events go in, events come out
 *
 */
class InputHandler
{
	typedef boost::signals2::signal<void (const SDL_Event&)> SignalType;
public:
	InputHandler();

	void PushEvent(const SDL_Event& ev);

	boost::signals2::connection AddHandler(SignalType::slot_function_type);

private:
	SignalType sig;
};

extern InputHandler input;

#endif // INPUT_HANDLER_H
