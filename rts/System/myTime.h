#ifndef MYTIME_H
#define MYTIME_H

#define SPRING_TIME 1 // boost microsec timer SUCKS atm, when it works again, set to 0
#if SPRING_TIME
#include <SDL_timer.h>
typedef unsigned spring_time;
typedef int spring_duration;
inline spring_time spring_gettime() { return SDL_GetTicks(); };
#define spring_tomsecs(time) (time)
#define spring_msecs(time) (time)
#define spring_secs(time) (time*1000)
#define spring_istime(time) (time>0)
#define spring_sleep(time) SDL_Delay(time)
#define spring_notime(time) time = 0
#else
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
using namespace boost::posix_time;
typedef ptime spring_time;
typedef time_duration spring_duration;
inline spring_time spring_gettime() { return microsec_clock::local_time(); };
#define spring_tomsecs(time) ((time).total_milliseconds())
#define spring_msecs(time) (milliseconds(time))
#define spring_secs(time) (seconds(time))
#define spring_istime(time) (!(time).is_not_a_date_time())
#define spring_sleep(time) boost::this_thread::sleep(time)
#define spring_notime(time)
#endif

#endif
