#define SPRING_TIME 1 // boost microsec timer SUCKS atm, when it works again, set to 0
#if SPRING_TIME
#include <SDL_timer.h>
#define spring_time unsigned
#define spring_duration int
#define spring_gettime() SDL_GetTicks()
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
#define spring_time ptime
#define spring_duration time_duration
#define spring_gettime() microsec_clock::local_time()
#define spring_tomsecs(time) ((time).total_milliseconds())
#define spring_msecs(time) (milliseconds(time))
#define spring_secs(time) (seconds(time))
#define spring_istime(time) (!(time).is_not_a_date_time())
#define spring_sleep(time) boost::this_thread::sleep(time)
#define spring_notime(time)
#endif
