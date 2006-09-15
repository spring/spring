// TimeProfiler.cpp: implementation of the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TimeProfiler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "LogOutput.h"
#include "SDL_timer.h"

#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"

#ifdef PROFILE_TIME

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTimeProfiler profiler;

CTimeProfiler::CTimeProfiler()
{
	lastBigUpdate=0;
	startTimeNum=0;
}

CTimeProfiler::~CTimeProfiler()
{

}

void CTimeProfiler::Draw()
{
	if(!gu->drawdebug)
		return;

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor4f(0,0,0.5f,0.5f);
	if(!profile.empty()){
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(0.65f,0.99f,0);
			glVertex3f(0.99f,0.99f,0);
			glVertex3f(0.65f,0.99f-profile.size()*0.024f-0.01f,0);
			glVertex3f(0.99f,0.99f-profile.size()*0.024f-0.01f,0);
		glEnd();
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	map<string,TimeRecord>::iterator pi;

	int y=0;
	for(pi=profile.begin();pi!=profile.end();++pi,y++)
		font->glPrintAt(0.655f, 0.960f-y*0.024f, 1.0f, "%20s %6.2fs %5.2f%%",pi->first.c_str(),((float)pi->second.total)/1000.f,pi->second.percent*100);

	glTranslatef(0.655f,0.965f,0);
	glScalef(0.015f,0.02f,0.02f);
	glColor4f(1,1,1,1);

	glDisable(GL_TEXTURE_2D);
	for(pi=profile.begin();pi!=profile.end();++pi){
		glColor3f(pi->second.color.x,pi->second.color.y,pi->second.color.z);
		glBegin(GL_QUADS);
			glVertex3f(0,0,0);
			glVertex3f(1,0,0);
			glVertex3f(1,1,0);
			glVertex3f(0,1,0);
		glEnd();
		if(!pi->second.showGraph){
			glColor3f(1,0,0);
			glBegin(GL_LINES);
				glVertex3f(0,0,0);
				glVertex3f(1,1,0);
				glVertex3f(1,0,0);
				glVertex3f(0,1,0);
			glEnd();
		}
		glTranslatef(0,-1.2f,0);
	}
	glPopMatrix();

	for(pi=profile.begin();pi!=profile.end();++pi){
		if(!pi->second.showGraph)
			continue;
		CVertexArray* va=GetVertexArray();
		va->Initialize();
		for(int a=0;a<128;++a){
			float p=((float)pi->second.frames[a])/1000.f*30;
			va->AddVertexT(float3(0.6f+a*0.003f,0.02f+p*0.4f,0),0,0);
		}
		glColor3f(pi->second.color.x,pi->second.color.y,pi->second.color.z);
		va->DrawArrayT(GL_LINE_STRIP);
	}
	glEnable(GL_TEXTURE_2D);
}

void CTimeProfiler::Update()
{
	map<string,TimeRecord>::iterator pi;
	for(pi=profile.begin();pi!=profile.end();++pi){
	  pi->second.frames[(gs->frameNum+1)&127]=0;
	}

	if(lastBigUpdate<gu->gameTime-1){
		lastBigUpdate=gu->gameTime;
		map<string,TimeRecord>::iterator pi;
		for(pi=profile.begin();pi!=profile.end();++pi){
			pi->second.percent=((float)pi->second.current)/1000.f;
			pi->second.current=0;

		}
	}
}

void CTimeProfiler::AddTime(string name, Sint64 time)
{
	map<string,TimeRecord>::iterator pi;
	if((pi=profile.find(name))!=profile.end()){
		pi->second.total+=time;
		pi->second.current+=time;
		pi->second.frames[gs->frameNum&127]+=time;
	} else {
		PUSH_CODE_MODE;
		ENTER_MIXED;
		profile[name].total=time;
		profile[name].current=time;
		profile[name].percent=0;
		for(int a=0;a<128;++a)
		  profile[name].frames[a]=0;
		profile[name].color.x=gu->usRandFloat();
		profile[name].color.y=gu->usRandFloat();
		profile[name].color.z=gu->usRandFloat();
		profile[name].showGraph=true;
		POP_CODE_MODE;
	}
}

void CTimeProfiler::StartTimer()
{
	if(startTimeNum==999){
		logOutput.Print("To many timers");
		return;
	}
	Uint64 starttime;
	starttime = SDL_GetTicks();
	startTimes[startTimeNum++]=(starttime);
}

void CTimeProfiler::EndTimer(char* name)
{
	if(startTimeNum==0)
		return;

	Uint64 stop;
	stop = SDL_GetTicks();
	AddTime(name,stop - startTimes[--startTimeNum]);
}

bool CTimeProfiler::IsAbove(int x, int y)
{
	float mx=float(x-gu->screenxPos)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(!gu->drawdebug || mx<0.65f || mx>0.99f || my<0.99f-profile.size()*0.024f-0.01f || my>0.99f)
		return false;

	return true;
}

bool CTimeProfiler::MousePress(int x, int y, int button)
{
	float mx=float(x-gu->screenxPos)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(!gu->drawdebug || mx<0.65f || mx>0.99f || my<0.99f-profile.size()*0.024f-0.01f || my>0.99f)
		return false;

	int num=(int) ((0.99f-my)/0.024f);

	int a=0;
	map<string,TimeRecord>::iterator pi;
	for(pi=profile.begin();pi!=profile.end() && a!=num;++pi,a++){
	}
	if(pi!=profile.end())
		pi->second.showGraph=!pi->second.showGraph;
	return false;
}

#endif
