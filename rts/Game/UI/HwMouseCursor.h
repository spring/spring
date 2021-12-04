/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HWMOUSECURSOR_H
#define HWMOUSECURSOR_H

#include "MouseCursor.h"

class IHardwareCursor {
public:
	static IHardwareCursor* Alloc(void*);
	static void Free(IHardwareCursor*);

	virtual ~IHardwareCursor() {}

	virtual void PushImage(int xsize, int ysize, const void* mem) = 0;
	virtual void PushFrame(int index, float delay) = 0;
	virtual void SetHotSpot(CMouseCursor::HotSpot hs) = 0;
	virtual void SetDelay(float delay) = 0;
	virtual void Finish() = 0;

	virtual bool NeedsYFlip() const = 0;
	virtual bool IsValid() const = 0;

	virtual void Init(CMouseCursor::HotSpot hs) = 0;
	virtual void Kill() = 0;
	virtual void Bind() = 0;
};

#endif /* HWMOUSECURSOR_H */
