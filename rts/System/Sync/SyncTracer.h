/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCTRACER_H
#define SYNCTRACER_H

//#define TRACE_SYNC


#include <fstream>
#include <deque>

class CSyncTracer  
{
	bool init();
public:
	void DeleteInterval();
	void NewInterval();
	void Commit();
	CSyncTracer();
	virtual ~CSyncTracer();

	CSyncTracer& operator<<(const char* c);
	CSyncTracer& operator<<(const int i);
	CSyncTracer& operator<<(const unsigned i);
	CSyncTracer& operator<<(const float f);

	std::ofstream* file;
	std::ofstream* logfile;
	std::string traces[10];
	int firstActive;
	int nowActive;
};

#ifdef TRACE_SYNC
extern CSyncTracer tracefile;
#endif

#endif /* SYNCTRACER_H */
