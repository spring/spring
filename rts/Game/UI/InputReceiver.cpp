#include "StdAfx.h"
#include "mmgr.h"

#include "InputReceiver.h"
#include "Rendering/GL/myGL.h"


float CInputReceiver::guiAlpha = 0.8f;

CInputReceiver* CInputReceiver::activeReceiver = NULL;


std::deque<CInputReceiver*>& GetInputReceivers()
{
	//This construct fixes order of initialization between different
	//compilation units using inputReceivers. (mantis # 34)
	static std::deque<CInputReceiver*> s_inputReceivers;
	return s_inputReceivers;
}

CInputReceiver::CInputReceiver(Where w)
{
	if (w == FRONT)
		GetInputReceivers().push_front(this);
	else if (w == BACK)
		GetInputReceivers().push_back(this);
}

CInputReceiver::~CInputReceiver()
{
	if (activeReceiver == this) {
		activeReceiver = NULL;
	}
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if(*ri==this){
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
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if (*ri == NULL) {
			inputReceivers.erase(ri);
			break;
		}
	}
}

CInputReceiver* CInputReceiver::GetReceiverAt(int x,int y)
{
	std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri) && (*ri)->IsAbove(x,y))
			return *ri;
	}
	return 0;
}

bool CInputReceiver::InBox(float x, float y,const ContainerBox& box)
{
	if(x>box.x1 && x<box.x2 && y>box.y1 && y<box.y2)
		return true;
	return false;
}

void CInputReceiver::DrawBox(const ContainerBox& box, int how)
{
	if (how == -1)
		how = GL_QUADS;
	glBegin(how);
	glVertex2f(box.x1, box.y1);
	glVertex2f(box.x1, box.y2);
	glVertex2f(box.x2, box.y2);
	glVertex2f(box.x2, box.y1);
	glEnd();
}

CInputReceiver::ContainerBox::ContainerBox()
: x1(0),
	x2(0),
	y1(0),
	y2(0)
{
}

CInputReceiver::ContainerBox CInputReceiver::ContainerBox::operator+(CInputReceiver::ContainerBox other)
{
	ContainerBox b;
	b.x1=x1+other.x1;
	b.x2=x1+other.x2;
	b.y1=y1+other.y1;
	b.y2=y1+other.y2;
	return b;
}
