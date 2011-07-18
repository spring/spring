/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNC_TRACER_H
#define SYNC_TRACER_H

//#define TRACE_SYNC

#include <fstream>
#include <string>

class CSyncTracer
{
	bool init();
public:
	CSyncTracer();
	virtual ~CSyncTracer();

	void DeleteInterval();
	void NewInterval();
	void Commit();

	CSyncTracer& operator<<(const std::string& s);
	CSyncTracer& operator<<(const char* value);
	CSyncTracer& operator<<(const int value);
	CSyncTracer& operator<<(const unsigned value);
	CSyncTracer& operator<<(const float value);

	std::ofstream* file;
	std::ofstream* logfile;
	std::string traces[10];
	int firstActive;
	int nowActive;
};

#ifdef TRACE_SYNC
extern CSyncTracer tracefile;
#endif

#endif /* SYNC_TRACER_H */
