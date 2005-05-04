// SyncTracer.cpp: implementation of the CSyncTracer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SyncTracer.h"
#include <stdio.h>
#include "infoconsole.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSyncTracer tracefile;

CSyncTracer::CSyncTracer()
{
	file=0;
	nowActive=0;
	firstActive=0;
}

CSyncTracer::~CSyncTracer()
{
#ifdef TRACE_SYNC
	delete file;
#endif

}

void CSyncTracer::Commit()
{
#ifdef TRACE_SYNC
	if(file==0){
		char c[100];
		sprintf(c,"trace%i.txt",gu->myTeam);
		file=new ofstream(c, ios::out);
	}
#endif
	(*file) << traces[firstActive].c_str();
	while(nowActive!=firstActive){
		firstActive++;
		if(firstActive==10)
			firstActive=0;
		(*file) << traces[firstActive].c_str();
	}
	traces[nowActive]="";
}

void CSyncTracer::NewInterval()
{
	nowActive++;
	if(nowActive==10)
		nowActive=0;
	traces[nowActive]="";
}

void CSyncTracer::DeleteInterval()
{
	if(firstActive!=nowActive)
		firstActive++;
	if(firstActive==10)
		firstActive=0;
}

CSyncTracer& CSyncTracer::operator<<(const char* c)
{
	traces[nowActive]+=c;
	(*info) << c;
	return *this;
}

CSyncTracer& CSyncTracer::operator<<(const int i)
{
	char t[20];
	sprintf(t,"%d",i);
	traces[nowActive]+=t;
	(*info) << i;
	return *this;
}

CSyncTracer& CSyncTracer::operator<<(const float f)
{
	char t[50];
	sprintf(t,"%f",f);
	traces[nowActive]+=t;
	(*info) << f;
	return *this;
}