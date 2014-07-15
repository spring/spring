/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CircularBuffer.h"



// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

CRangeLocker::CRangeLocker(bool _cpuUpdates)
: mCPUUpdates(_cpuUpdates)
{
}


CRangeLocker::~CRangeLocker()
{
	for (auto& it: mBufferLocks) {
		glDeleteSync(it.second.mSyncObj);
		glDeleteFencesNV(1, &it.second.nvFence);
	}
}

// --------------------------------------------------------------------------------------------------------------------

#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

void CRangeLocker::WaitForLockedRange(size_t _lockBeginBytes, size_t _lockLength)
{
	if (_lockLength == 0)
		return;

	SCOPED_TIMER("WaitForLockedRange1");
	for (auto it = mBufferLocks.begin(); it != mBufferLocks.end();) {
		if (it->second.Overlaps(_lockBeginBytes, _lockLength)) {
			wait(it->second.mSyncObj, it->second.nvFence);
			//glDeleteSync(it->second.mSyncObj);
			glDeleteFencesNV(1, &it->second.nvFence);
			SCOPED_TIMER("WaitForLockedRange2");
			it = mBufferLocks.erase(it);
		} else {
			++it;
		}
	}
}


void CRangeLocker::LockRange(size_t _lockBeginBytes, size_t _lockLength)
{
	assert(_lockLength > 0);
	BufferLock& bl = mBufferLocks[_lockBeginBytes];
	bl.mLength       = _lockLength;
	bl.mStartOffset  = _lockBeginBytes;
	//if (bl.mSyncObj != nullptr) glDeleteSync(bl.mSyncObj);
	SCOPED_TIMER("glFenceSync");
	//bl.mSyncObj      = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	assert(mBufferLocks.size() < 100);

	if (bl.nvFence == 0) glGenFencesNV(1, &bl.nvFence);
	glSetFenceNV(bl.nvFence, GL_ALL_COMPLETED_NV);
}

// --------------------------------------------------------------------------------------------------------------------

void CRangeLocker::garbagecollect(GLsync exclude)
{
	for (auto it = mBufferLocks.begin(); it != mBufferLocks.end();) {
		if (it->second.mSyncObj == exclude) {
			++it;
			continue;
		}

		GLenum waitRet = glClientWaitSync(it->second.mSyncObj, 0, 0);
		if (waitRet != GL_TIMEOUT_EXPIRED) {
			glDeleteSync(it->second.mSyncObj);
			it = mBufferLocks.erase(it);
		} else {
			++it;
		}
	}
}


void CRangeLocker::wait(GLsync _syncObj, GLuint nvFence)
{
	{
		glFinishFenceNV(nvFence);
		return;
	}

	if (mCPUUpdates) {
		static GLuint64 kOneSecondInNanoSeconds = 1000000000;

		GLbitfield waitFlags  = 0;
		GLuint64 waitDuration = 0;
		while (true) {
			SCOPED_TIMER("WaitForLockedRange4");
			GLenum waitRet = glClientWaitSync(_syncObj, waitFlags, waitDuration);
			if (waitRet != GL_TIMEOUT_EXPIRED)
				return;

			// After the first time, need to start flushing, and wait for a looong time.
			waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
			waitDuration = kOneSecondInNanoSeconds;

			//garbagecollect(_syncObj);
		}
	} else {
		glWaitSync(_syncObj, 0, GL_TIMEOUT_IGNORED);
	}
}
