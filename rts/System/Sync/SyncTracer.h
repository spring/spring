/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNC_TRACER_H
#define SYNC_TRACER_H

#ifdef TRACE_SYNC

#include <fstream>

class CSyncTracer
{
public:
	void Initialize(int playerIndex);

	template<typename T>
	CSyncTracer& operator<<(const T& value)
	{
		if (logfile.good()) {
			logfile << value;
		}
		return *this;
	}

private:
	std::ofstream logfile;
};

extern CSyncTracer tracefile;

#endif // TRACE_SYNC

#endif // SYNC_TRACER_H
