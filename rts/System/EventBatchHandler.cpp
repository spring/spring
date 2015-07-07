/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Util.h"

#include "System/EventBatchHandler.h"
#include "System/EventHandler.h"

// managed by EventHandler
EventBatchHandler* EventBatchHandler::ebh = nullptr;

void EventBatchHandler::CreateInstance()
{
	if (ebh == nullptr) {
		ebh = new EventBatchHandler();
	}
}

void EventBatchHandler::DeleteInstance()
{
	if (ebh != nullptr)	{
		delete ebh;
		ebh = nullptr;
	}
}



EventBatchHandler::EventBatchHandler()
	: CEventClient("[EventBatchHandler]", 0, true)
{
	autoLinkEvents = true;
	RegisterLinkedEvents(this);
	eventHandler.AddClient(this);
}
