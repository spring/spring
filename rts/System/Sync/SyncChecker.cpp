/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef SYNCCHECK

#include "SyncChecker.h"

// This cannot be included in the header file (SyncChecker.h) because include conflicts will occur.
#include "System/Threading/ThreadPool.h"


unsigned CSyncChecker::g_checksum;
int CSyncChecker::inSyncedCode;


void CSyncChecker::debugSyncCheckThreading()
{
    assert(ThreadPool::GetThreadNum() == 0);
}

#endif // SYNCDEBUG
