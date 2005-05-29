#ifndef SYNCTRACER_H
#define SYNCTRACER_H
// SyncTracer.h: interface for the CSyncTracer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNCTRACER_H__D2843FC4_B49B_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_SYNCTRACER_H__D2843FC4_B49B_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

//#define TRACE_SYNC


#include <fstream>
#include <deque>

using namespace std;

class CSyncTracer  
{
public:
	void DeleteInterval();
	void NewInterval();
	void Commit();
	CSyncTracer();
	virtual ~CSyncTracer();

	CSyncTracer& operator<<(const char* c);
	CSyncTracer& operator<<(const int i);
	CSyncTracer& operator<<(const float f);
	
	ofstream* file;
	std::string traces[10];
	int firstActive;
	int nowActive;
};

#ifdef TRACE_SYNC
extern CSyncTracer tracefile;
#endif

#endif // !defined(AFX_SYNCTRACER_H__D2843FC4_B49B_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* SYNCTRACER_H */
