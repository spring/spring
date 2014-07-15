/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "myGL.h"
#include <unordered_map>

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

class CRangeLocker
{
public:
	CRangeLocker(bool _cpuUpdates);
	~CRangeLocker();

	void WaitForLockedRange(size_t _lockBeginBytes, size_t _lockLength);
	void LockRange(size_t _lockBeginBytes, size_t _lockLength);

private:
	void wait(GLsync _syncObj, GLuint nvFence);
	void garbagecollect(GLsync exclude = nullptr);

private:
	struct BufferLock
	{
		size_t mStartOffset;
		size_t mLength;
		GLsync mSyncObj;
		GLuint nvFence;

		BufferLock()
		: mStartOffset(0)
		, mLength(0)
		, mSyncObj(nullptr)
		, nvFence(0)
		{}

		bool Overlaps(const size_t startOffset, const size_t length) const {
			return mStartOffset < (startOffset + length)
				&& startOffset < (mStartOffset + mLength);
		}
	};

	std::unordered_map<size_t, BufferLock> mBufferLocks;

	// Whether it's the CPU (true) that updates, or the GPU (false)
	bool mCPUUpdates;
};


#endif // CIRCULARBUFFER_H
