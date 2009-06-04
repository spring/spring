#include "ThreadSafeContainers.h"

#ifndef USE_GML

/*CR_BIND_TEMPLATE(ThreadListSimRender, )
CR_REG_METADATA(ThreadListSimRender,(
	CR_MEMBER(cont)
));*/

#else

/*CR_BIND_TEMPLATE(ThreadListSimRender, )
CR_REG_METADATA(ThreadListSimRender,(
	CR_MEMBER(contSim),
	CR_POSTLOAD(PostLoad)
));*/


void ThreadListSimRender::PostLoad()
{
	for (SimIT it = contSim.begin(); it != contSim.end(); it++) {
		addRender.push_back(*it);
	}
}

void ThreadVectorSimRender::PostLoad()
{
	for (SimIT it = contSim.begin(); it != contSim.end(); it++) {
		addRender.push_back(*it);
	}
}

#endif