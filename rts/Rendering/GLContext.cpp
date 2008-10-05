// GLContext.cpp: implementation of the GLContext namespace.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef free
#undef free
#endif

#include "GLContext.h"

#include <list>


using namespace std;

struct HookSet {
	HookSet()
	: init(NULL), free(NULL), data(NULL) {}

	HookSet(GLContext::Func i, GLContext::Func f, void* d)
	: init(i), free(f), data(d) {}

	bool operator==(const HookSet& hs) const
	{
		return ((init == hs.init) && (free == hs.free) && (data == hs.data));
	}

	GLContext::Func init;
	GLContext::Func free;
	void* data;
};


static list<HookSet>& getHooks()
{
	static list<HookSet> hooks; // local flow for proper initialization
	return hooks;
}


void GLContext::Init()
{
	list<HookSet>::iterator it;
	for (it = getHooks().begin(); it != getHooks().end(); ++it) {
		it->init(it->data);
	}
}


void GLContext::Free()
{
	list<HookSet>::iterator it;
	for (it = getHooks().begin(); it != getHooks().end(); ++it) {
		it->free(it->data);
	}
}


void GLContext::InsertHookSet(Func init, Func free, void* data)
{
	if ((init == NULL) || (free == NULL)) {
		return;
	}
	HookSet hs(init, free, data);
	getHooks().push_back(hs);
}


void GLContext::RemoveHookSet(Func init, Func free, void* data)
{
	HookSet hs(init, free, data);
	list<HookSet>::iterator it;
	for (it = getHooks().begin(); it != getHooks().end(); ++it) {
		if (hs == *it) {
			list<HookSet>::iterator it_next = it;
			it_next++;
			getHooks().erase(it);
			it = it_next;
		}
	}
}
