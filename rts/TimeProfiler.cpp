// TimeProfiler.cpp: implementation of the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TimeProfiler.h"
#include "myGL.h"
#include "glFont.h"
#include "InfoConsole.h"

#include "VertexArray.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTimeProfiler profiler;

CTimeProfiler::CTimeProfiler()
{
	QueryPerformanceFrequency(&timeSpeed);
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
			glVertex3f(0.65f,0.99f-profile.size()*0.024-0.01,0);
			glVertex3f(0.99f,0.99f-profile.size()*0.024-0.01,0);
		glEnd();
	}
	glTranslatef(0.655f,0.965f,0);
	glScalef(0.015f,0.02f,0.02f);
	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);

	map<string,TimeRecord>::iterator pi;

	glPushMatrix();
	for(pi=profile.begin();pi!=profile.end();++pi){
#ifdef NO_WINSTUFF
		font->glPrint("%20s %6.2fs %5.2f%%",pi->first.c_str(),((double)pi->second.total)/timeSpeed,pi->second.percent*100);
#else
		font->glPrint("%20s %6.2fs %5.2f%%",pi->first.c_str(),((double)pi->second.total.QuadPart)/timeSpeed.QuadPart,pi->second.percent*100);
#endif

		glTranslatef(0,-1.2f,0);

	}
	glPopMatrix();

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
#ifdef NO_WINSTUFF
			float p=((double)pi->second.frames[a])/timeSpeed*30;
#else
			float p=((double)pi->second.frames[a].QuadPart)/timeSpeed.QuadPart*30;
#endif
			va->AddVertexT(float3(0.6+a*0.003,0.02+p*0.4,0),0,0);
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
#ifdef NO_WINSTUFF
	  pi->second.frames[(gs->frameNum+1)&127]=0;
#else
	  pi->second.frames[(gs->frameNum+1)&127].QuadPart=0;
#endif
	}

	if(lastBigUpdate<gu->gameTime-1){
		lastBigUpdate=gu->gameTime;
		map<string,TimeRecord>::iterator pi;
		for(pi=profile.begin();pi!=profile.end();++pi){
#ifndef NO_WINSTUFF
			pi->second.percent=((double)pi->second.current.QuadPart)/timeSpeed.QuadPart;
			pi->second.current.QuadPart=0;
#else 
			pi->second.percent=((double)pi->second.current)/timeSpeed;
			pi->second.current=0;
#endif

		}
	}
}

void CTimeProfiler::AddTime(string name, __int64 time)
{
	map<string,TimeRecord>::iterator pi;
	if((pi=profile.find(name))!=profile.end()){
#ifndef NO_WINSTUFF
		pi->second.total.QuadPart+=time;
		pi->second.current.QuadPart+=time;
		pi->second.frames[gs->frameNum&127].QuadPart+=time;
#else
		pi->second.total+=time;
		pi->second.current+=time;
		pi->second.frames[gs->frameNum&127]+=time;
#endif
	} else {
		PUSH_CODE_MODE;
		ENTER_MIXED;
#ifndef NO_WINSTUFF
		profile[name].total.QuadPart=time;
		profile[name].current.QuadPart=time;
#else
		profile[name].total=time;
		profile[name].current=time;
#endif
		profile[name].percent=0;
		for(int a=0;a<128;++a)
#ifndef  NO_WINSTUFF
		  profile[name].frames[a].QuadPart=0;
#else
		  profile[name].frames[a]=0;
#endif
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
		info->AddLine("To many timers");
		return;
	}
	LARGE_INTEGER starttime;
#ifndef NO_WINSTUFF
	QueryPerformanceCounter(&starttime);
	startTimes[startTimeNum++]=(starttime.QuadPart);
#endif
}

void CTimeProfiler::EndTimer(char* name)
{
	if(startTimeNum==0)
		return;

	LARGE_INTEGER stop;
#ifndef NO_WINSTUFF
	QueryPerformanceCounter(&stop);
	AddTime(name,stop.QuadPart - startTimes[--startTimeNum]);
#else
	AddTime(name,stop - startTimes[--startTimeNum]);
#endif
}

bool CTimeProfiler::IsAbove(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(!gu->drawdebug || mx<0.65 || mx>0.99 || my<0.99f-profile.size()*0.024-0.01 || my>0.99)
		return false;

	return true;
}

bool CTimeProfiler::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(!gu->drawdebug || mx<0.65 || mx>0.99 || my<0.99f-profile.size()*0.024-0.01 || my>0.99)
		return false;

	int num=(0.99-my)/0.024;

	int a=0;
	map<string,TimeRecord>::iterator pi;
	for(pi=profile.begin();pi!=profile.end() && a!=num;++pi,a++){
	}
	if(pi!=profile.end())
		pi->second.showGraph=!pi->second.showGraph;
	return false;
}
