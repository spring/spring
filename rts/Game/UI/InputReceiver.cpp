/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "InputReceiver.h"
#include "Rendering/GL/myGL.h"


float CInputReceiver::guiAlpha = 0.8f;

CInputReceiver* CInputReceiver::activeReceiver = NULL;


std::list<CInputReceiver*>& GetInputReceivers()
{
	// This construct fixes order of initialization between different
	// compilation units using inputReceivers. (mantis # 34)
	static std::list<CInputReceiver*> s_inputReceivers;
	return s_inputReceivers;
}

CInputReceiver::CInputReceiver(Where w)
{
	switch (w) {
		case FRONT: {
			GetInputReceivers().push_front(this);
			break;
		}
		case BACK: {
			GetInputReceivers().push_back(this);
			break;
		}
		default: break;
	}
}

CInputReceiver::~CInputReceiver()
{
	if (activeReceiver == this) {
		activeReceiver = NULL;
	}
	std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::list<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		if (*ri == this) {
			// we may be deleted while there are still iterators active
			//inputReceivers.erase(ri);
			*ri = NULL;
			break;
		}
	}
}

void CInputReceiver::CollectGarbage()
{
	// erase one NULL element each call (should be enough for now)
	// called once every sec from CGame::Update
	std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::list<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		if (*ri == NULL) {
			inputReceivers.erase(ri);
			break;
		}
	}
}

CInputReceiver* CInputReceiver::GetReceiverAt(int x,int y)
{
	std::list<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::list<CInputReceiver*>::iterator ri;
	for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
		CInputReceiver* recv = *ri;
		if (recv && recv->IsAbove(x,y)) {
			return recv;
		}
	}
	return NULL;
}

bool CInputReceiver::InBox(float x, float y, const ContainerBox& box) const
{
	return ((x > box.x1) &&
			(x < box.x2) &&
			(y > box.y1) &&
			(y < box.y2));
}

void CInputReceiver::DrawBox(const ContainerBox& box, int how)
{
	if (how == -1) {
		how = GL_QUADS;
	}
	glBegin(how);
	glVertex2f(box.x1, box.y1);
	glVertex2f(box.x1, box.y2);
	glVertex2f(box.x2, box.y2);
	glVertex2f(box.x2, box.y1);
	glEnd();
}

CInputReceiver::ContainerBox::ContainerBox()
	: x1(0.0f)
	, y1(0.0f)
	, x2(0.0f)
	, y2(0.0f)
{
}

CInputReceiver::ContainerBox CInputReceiver::ContainerBox::operator+(CInputReceiver::ContainerBox other) const
{
	ContainerBox b;
	b.x1 = x1 + other.x1;
	b.x2 = x1 + other.x2;
	b.y1 = y1 + other.y1;
	b.y2 = y1 + other.y2;
	return b;
}
