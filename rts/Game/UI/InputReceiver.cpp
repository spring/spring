/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "InputReceiver.h"
#include "Lua/LuaInputReceiver.h"
#include "Rendering/GL/myGL.h"


float CInputReceiver::guiAlpha = 0.8f;

CInputReceiver* CInputReceiver::activeReceiver = nullptr;

CInputReceiver::CInputReceiver(Where w)
{
	switch (w) {
		case FRONT: {
			GetReceivers().push_front(this);
		} break;
		case BACK: {
			GetReceivers().push_back(this);
		} break;
		default: break;
	}
}

CInputReceiver::~CInputReceiver()
{
	if (activeReceiver == this)
		activeReceiver = nullptr;

	for (CInputReceiver*& r: GetReceivers()) {
		if (r == this) {
			// we may be deleted while there are still iterators active
			// inputReceivers.erase(ri);
			r = nullptr;
			break;
		}
	}
}

void CInputReceiver::CollectGarbage()
{
	// called once every second from CGame::Update
	std::deque<CInputReceiver*>& prvInputReceivers = GetReceivers();
	std::deque<CInputReceiver*> nxtInputReceivers;

	for (CInputReceiver* r: prvInputReceivers) {
		if (r != nullptr) {
			nxtInputReceivers.push_back(r);
		}
	}

	prvInputReceivers.swap(nxtInputReceivers);
}

CInputReceiver* CInputReceiver::GetReceiverAt(int x,int y)
{
	if (luaInputReceiver != nullptr && luaInputReceiver->IsAbove(x,y))
		return luaInputReceiver;

	for (CInputReceiver* recv: GetReceivers()) {
		if (recv != nullptr && recv->IsAbove(x,y)) {
			return recv;
		}
	}

	return nullptr;
}

bool CInputReceiver::InBox(float x, float y, const ContainerBox& box) const
{
	return ((x > box.x1) && (x < box.x2)  &&  (y > box.y1) && (y < box.y2));
}

void CInputReceiver::DrawBox(const ContainerBox& box, int polyMode)
{
	if (polyMode == -1)
		polyMode = GL_QUADS;

	glBegin(polyMode);
	glVertex2f(box.x1, box.y1);
	glVertex2f(box.x1, box.y2);
	glVertex2f(box.x2, box.y2);
	glVertex2f(box.x2, box.y1);
	glEnd();
}

