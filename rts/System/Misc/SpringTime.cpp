/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpringTime.h"

#ifdef STATIC_SPRING_TIME

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


#endif
