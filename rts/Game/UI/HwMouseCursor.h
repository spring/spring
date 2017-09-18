/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HWMOUSECURSOR_H
#define HWMOUSECURSOR_H

#include <string>
#include <vector>
#include "MouseCursor.h"

class CMouseCursor;

class IHwCursor {
	public:
		virtual void PushImage(int xsize, int ysize, const void* mem) = 0;
		virtual void SetDelay(float delay) = 0;
		virtual void PushFrame(int index, float delay) = 0;
		virtual void Finish() = 0;

		virtual bool NeedsYFlip() = 0; //windows needs flipped Y axis

		virtual bool IsValid() = 0;
		virtual void Bind() = 0;

		virtual ~IHwCursor() {};

		CMouseCursor::HotSpot hotSpot;
};

IHwCursor* GetNewHwCursor();

#endif /* HWMOUSECURSOR_H */
