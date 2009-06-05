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


template<class T>
void ThreadListSimRender<T>::PostLoad()
{
	for (SimIT it = contSim.begin(); it != contSim.end(); it++) {
		addRender.push_back(*it);
	}
}

template<class T>
void ThreadVectorSimRender<T>::PostLoad()
{
	for (SimIT it = contSim.begin(); it != contSim.end(); it++) {
		addRender.push_back(*it);
	}
}

#endif