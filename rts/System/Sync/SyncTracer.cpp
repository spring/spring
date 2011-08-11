/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef TRACE_SYNC

#include "SyncTracer.h"
#include "System/Log/ILog.h"

#include <cstdio>

CSyncTracer tracefile;

void CSyncTracer::Initialize(int playerIndex)
{
	logfile.close();
	logfile.clear();

	char newLogFileName[64];
	sprintf(newLogFileName, "trace%i.log", playerIndex);
	logfile.open(newLogFileName);
	LOG("Sync trace log: %s", newLogFileName);
}

#endif // TRACE_SYNC
