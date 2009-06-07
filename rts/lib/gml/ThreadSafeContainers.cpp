#include "ThreadSafeContainers.h"

#ifndef USE_GML

/*CR_BIND_TEMPLATE(ThreadListSimRender, )
CR_REG_METADATA(ThreadListSimRender,(
	CR_MEMBER(cont)
));*/

#else

/*CR_BIND_TEMPLATE(ThreadListSimRender, )
CR_REG_METADATA(ThreadListSimRender,(
	CR_MEMBER(cont),
	CR_POSTLOAD(PostLoad)
));*/

#endif