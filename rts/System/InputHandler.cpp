#include "InputHandler.h"


InputHandler input;


InputHandler::InputHandler()
{
}

void InputHandler::PushEvent(const SDL_Event& ev)
{
	for (std::list<HandlerFunc>::const_iterator it = handlers.begin(); it != handlers.end(); ++it)
	{
		if ((*it)(ev))
		{
			return;
		}
	}
}

void InputHandler::AddHandler(HandlerFunc func)
{
	for (std::list<HandlerFunc>::const_iterator it = handlers.begin(); it != handlers.end(); ++it)
	{
		if (it == func)
		{
			// not adding twice
			return;
		}
	}
	handlers.push_back(func);
}

void InputHandler::RemoveHandler(HandlerFunc func)
{
	for (std::list<HandlerFunc>::iterator it = handlers.begin(); it != handlers.end(); ++it)
	{
		if (it == func)
		{
			handlers.erase(it);
		}
	}
}

