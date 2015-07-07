/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EVENT_BATCH_HANDLER_HDR
#define EVENT_BATCH_HANDLER_HDR

#include "EventClient.h"


class EventBatchHandler : public CEventClient {
public: // EventClient
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return CEventClient::AllAccessTeam; }
	
	//void LoadedModelRequested()

public:
	static EventBatchHandler* GetInstance() { assert(ebh); return ebh; }
	static void CreateInstance();
	static void DeleteInstance();

	EventBatchHandler();
	virtual ~EventBatchHandler() {}

private:
	static EventBatchHandler* ebh;

};

#define eventBatchHandler (EventBatchHandler::GetInstance())

#endif
