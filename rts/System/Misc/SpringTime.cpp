/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpringTime.h"
#include "System/Log/ILog.h"

#ifdef USING_CREG
#include "System/creg/Serializer.h"

//FIXME always use class even in non-debug! for creg!
CR_BIND(spring_time, );
CR_REG_METADATA(spring_time,(
	CR_IGNORED(x),
	CR_SERIALIZER(Serialize)
));
#endif

void spring_time::Serialize(creg::ISerializer& s)
{
	if (s.IsWriting()) {
		int y = spring_tomsecs(*this - spring_gettime());
		s.SerializeInt(&y, 4);
	} else {
		int y;
		s.SerializeInt(&y, 4);
		*this = *this + spring_msecs(y);
	}
}


void spring_time::sleep() {
	//FIXME for very short time intervals use a yielding loop instead? (precision of yield is like 5x better than sleep, see the UT)
	//assert(toNanoSecs() > spring_msecs(1).toNanoSecs());

	const spring_time expectedWakeUpTime = gettime() + *this;
	#if defined(SPRINGTIME_USING_STDCHRONO)
		this_thread::sleep_for(chrono::nanoseconds( toNanoSecs() ));
	#else
		boost::this_thread::sleep(boost::posix_time::microseconds(std::ceil(toNanoSecsf() * 1e-3)));
	#endif
	const float diffMs = (gettime() - expectedWakeUpTime).toMilliSecsf();
	if (diffMs > 7.0f) {
		LOG_L(L_WARNING, "SpringTime: used sleep() function is too inaccurate");
	}
}
