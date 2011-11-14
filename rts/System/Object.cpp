/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/Object.h"
#include "System/mmgr.h"
#include "System/creg/STL_Set.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"

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
	for (std::map<DependenceType, std::list<CObject*> >::iterator i = listeners.begin(); i != listeners.end(); ++i) {
		const DependenceType& depType = i->first;
		std::list<CObject*>& objs = i->second;

		for (std::list<CObject*>::iterator di = objs.begin(); di != objs.end(); ++di) {
			CObject*& obj = (*di);
			
			m_setOwner(__FILE__, __LINE__, __FUNCTION__);
			obj->DependentDied(this);
			m_setOwner(__FILE__, __LINE__, __FUNCTION__);

			assert(obj->listening.find(depType) != obj->listening.end());
			ListErase<CObject*>(obj->listening[depType], this);
		}
	}
	for (std::map<DependenceType, std::list<CObject*> >::iterator i = listening.begin(); i != listening.end(); ++i) {
		const DependenceType& depType = i->first;
		std::list<CObject*>& objs = i->second;

		for (std::list<CObject*>::iterator di = objs.begin(); di != objs.end(); ++di) {
			CObject*& obj = (*di);

			m_setOwner(__FILE__, __LINE__, __FUNCTION__);
			assert(obj->listeners.find(depType) != obj->listeners.end());
			ListErase<CObject*>(obj->listeners[depType], this);
		}
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
		int num = listening.size();
		ser->Serialize(&num, sizeof(int));
		for (std::map<DependenceType, std::list<CObject*> >::iterator i = listening.begin(); i != listening.end(); ++i) {
			int dt = i->first;
			ser->Serialize(&dt, sizeof(int));
			std::list<CObject*>& dl = i->second;
			int size = 0;
			std::list<CObject*>::const_iterator oi;
			for (oi = dl.begin(); oi != dl.end(); ++oi) {
				if ((*oi)->GetClass() != CObject::StaticClass()) {
					size++;
				}
			}
			ser->Serialize(&size, sizeof(int));
			for (oi = dl.begin(); oi != dl.end(); ++oi) {
				if ((*oi)->GetClass() != CObject::StaticClass()) {
					ser->SerializeObjectPtr((void**)&*oi, (*oi)->GetClass());
				} else {
					LOG("Death dependance not serialized in %s",
						GetClass()->name.c_str());
				}
			}
		}
	} else {
		int num;
		ser->Serialize(&num, sizeof(int));
		for (int i = 0; i < num; i++) {
			int dt;
			ser->Serialize(&dt, sizeof(int));
			int size;
			ser->Serialize(&size, sizeof(int));
			std::list<CObject*>& dl = listening[(DependenceType)dt];
			for (int o = 0; o < size; o++) {
				std::list<CObject*>::iterator oi =
					dl.insert(dl.end(), NULL);
				ser->SerializeObjectPtr((void**)&*oi, NULL);
			}
		}
	}
}

void CObject::PostLoad()
{
	for (std::map<DependenceType, std::list<CObject*> >::iterator i = listening.begin(); i != listening.end(); ++i) {
		for (std::list<CObject*>::iterator oi = i->second.begin(); oi != i->second.end(); ++oi) {
			m_setOwner(__FILE__, __LINE__, __FUNCTION__);
			std::list<CObject*>& dl = (*oi)->listeners[i->first];
			dl.insert(dl.end(), this);
		}
	}
	m_resetGlobals();
}

void CObject::DependentDied(CObject* obj)
{
}

// NOTE that we can be listening to a single object from several different places,
// however objects are responsible for not adding the same dependence more than once,
// and preferably try to delete the dependence asap in order not to waste memory
void CObject::AddDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	if (!ListInsert<CObject *>(obj->listeners[dep], this)) {
#ifndef NDEBUG
		LOG_L(L_ERROR, "AddDeathDependence: Duplicated listener object for dependence type %d", (int)dep);
		CrashHandler::OutputStacktrace();
#endif
	}

	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	if (!ListInsert<CObject *>(listening[dep], obj)) {
#ifndef NDEBUG
		LOG_L(L_ERROR, "AddDeathDependence: Duplicated listening object for dependence type %d", (int)dep);
		CrashHandler::OutputStacktrace();
#endif
	}
	m_resetGlobals();
}

void CObject::DeleteDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	if (!ListErase<CObject *>(listening[dep], obj)) {
#ifndef NDEBUG
		LOG_L(L_ERROR, "DeleteDeathDependence: Non existent listening object for dependence type %d", (int)dep);
		CrashHandler::OutputStacktrace();
#endif
	}

	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	if (!ListErase<CObject *>(obj->listeners[dep], this)) {
#ifndef NDEBUG
		LOG_L(L_ERROR, "DeleteDeathDependence: Non existent listener object for dependence type %d", (int)dep);
		CrashHandler::OutputStacktrace();
#endif
	}
	m_resetGlobals();
}
