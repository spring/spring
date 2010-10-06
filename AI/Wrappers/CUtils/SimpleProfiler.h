/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMPLE_PROFILER_H
#define SIMPLE_PROFILER_H

#ifdef	__cplusplus
extern "C" {
#endif

#if       !defined PROFILING_ENABLED && defined DEBUG
	// Always enable profiling in debug binaries
	#define PROFILING_ENABLED 1
#endif // !defined PROFILING_ENABLED && defined DEBUG

/**
 * Adds a specified ammount of time (in milli-seconds) to a certain part
 */
void          simpleProfiler_addTime(const char* const part, unsigned time);

/**
 * Returns the total ammount of time (in milli-seconds)
 * spent in a certain part so far
 */
unsigned long simpleProfiler_getTime(const char* const part);

/**
 * Returns the total number of profiling parts
 */
unsigned      simpleProfiler_getParts();

/**
 * Returns the name of a certain part by index.
 * @see simpleProfiler_getParts()
 */
const char*   simpleProfiler_getPartName(unsigned partIndex);

/**
 * Allows fetching the list of names of the profiling parts.
 * @return number of parts
 */
unsigned      simpleProfiler_getPartNames(const char** parts, const unsigned parts_sizeMax);

/**
 * Returns a summary string, consisting or name and total-time
 * of all profiling parts, each on a separate line.
 * Note: You have to free(...) the return value.
 */
char*         simpleProfiler_getSummaryString();

#ifdef __cplusplus
} // extern "C"
#endif


#if       defined __cplusplus

#if       defined PROFILING_ENABLED
	#define SCOPED_TIMER(part, profiler) ScopedTimer myScopedTimerFromMakro(part, profiler);
#else  // defined PROFILING_ENABLED
	#define SCOPED_TIMER(part, profiler)
#endif // defined PROFILING_ENABLED

#define DEFAULT_SCOPED_TIMER(part) SCOPED_TIMER(part, Profiler::GetDefault());


#include <map>
#include <string>

class Profiler
{
public:
	static Profiler* GetDefault();

	Profiler(const char* const name);

	const char* GetName() const;

	void AddTime(const char* const part, unsigned long time);
	const std::map<const char* const, unsigned long>& GetParts();

	std::string ToString() const;

private:
	const char* const name;
	std::map<const char* const, unsigned long> parts;

	static Profiler def;
};

/**
 * @brief Time profiling helper class
 *
 * Construct an instance of this class where you want to begin time measuring,
 * and destruct it at the end (or let it be autodestructed).
 */
class ScopedTimer
{
public:
	ScopedTimer(const char* const part, Profiler* profiler = Profiler::GetDefault());
	/**
	 * @brief destroy and add time to profiler
	 */
	~ScopedTimer();

private:
	const char* const part;
	Profiler* profiler;
	/// in milli-seconds
	unsigned long startTime;
};

#endif // defined __cplusplus

#endif // SIMPLE_PROFILER_H
