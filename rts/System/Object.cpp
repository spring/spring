/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/Object.h"
#include "System/mmgr.h"
#include "System/creg/STL_Set.h"
#include "System/Log/ILog.h"

#ifndef USE_MMGR
# define m_setOwner(file, line, func)
# define m_resetGlobals()
#endif

CR_BIND(CObject, )

CR_REG_METADATA(CObject, (
//	CR_MEMBER(listening),
//	CR_MEMBER(listeners),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CObject::CObject() : detached(false)
{
}

void CObject::Detach()
{
	// SYNCED
	assert(!detached);
	detached = true;
	std::list<CObject*>::iterator di;
	for (di = listeners.begin(); di != listeners.end(); ++di) {
 		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		(*di)->DependentDied(this);
		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		ListErase<CObject*>((*di)->listening, this);
	}
	for (di = listening.begin(); di != listening.end(); ++di) {
		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		ListErase<CObject*>((*di)->listeners, this);
	}
	m_resetGlobals();
}


CObject::~CObject()
{
	// UNSYNCED (if detached)
	if (!detached)
		Detach();
}

void CObject::Serialize(creg::ISerializer* ser)
{
	if (ser->IsWriting()) {
		int size = 0;
		std::list<CObject*>::const_iterator oi;
		for (oi = listening.begin(); oi != listening.end(); ++oi) {
			if ((*oi)->GetClass() != CObject::StaticClass()) {
				size++;
			}
		}
		ser->Serialize(&size, sizeof(int));
		for (oi = listening.begin(); oi != listening.end(); ++oi) {
			if ((*oi)->GetClass() != CObject::StaticClass()) {
				ser->SerializeObjectPtr((void**)&*oi, (*oi)->GetClass());
			} else {
				LOG("Death dependance not serialized in %s",
						GetClass()->name.c_str());
			}
		}
	} else {
		int size;
		ser->Serialize(&size, sizeof(int));
		for (int o = 0; o < size; o++) {
			std::list<CObject*>::iterator oi =
					listening.insert(listening.end(), NULL);
			ser->SerializeObjectPtr((void**)&*oi, NULL);
		}
	}
}

void CObject::PostLoad()
{
	std::list<CObject*>::iterator oi;
	for (oi = listening.begin(); oi != listening.end(); ++oi) {
		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		(*oi)->listeners.insert((*oi)->listeners.end(), this);
	}
	m_resetGlobals();
}

void CObject::DependentDied(CObject* obj)
{
}

void CObject::AddDeathDependence(CObject* obj)
{
	assert(!detached);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	obj->listeners.insert(obj->listeners.end(), this);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	listening.insert(listening.end(), obj);
	m_resetGlobals();
}

void CObject::DeleteDeathDependence(CObject* obj)
{
	assert(!detached);
	// NOTE that we can be listening to a single object from several different
	// places (like curReclaim in CBuilder and lastAttacker in CUnit, grr)
	// so we should only remove one of them
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	ListErase<CObject*>(listening, obj);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	ListErase<CObject*>(obj->listeners, this);
	m_resetGlobals();
}
