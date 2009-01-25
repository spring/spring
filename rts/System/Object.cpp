#include "StdAfx.h"
// Object.cpp: implementation of the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
#include "mmgr.h"
#include "creg/STL_Set.h"
#include "LogOutput.h"

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

CObject::CObject()
{
}

CObject::~CObject()
{
	std::list<CObject*>::iterator di;
	for(di=listeners.begin();di!=listeners.end();++di){
 		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		(*di)->DependentDied(this);
		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		ListErase<CObject*>((*di)->listening, this);
	}
	for(di=listening.begin();di!=listening.end();++di){
		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		ListErase<CObject*>((*di)->listeners, this);
	}
	m_resetGlobals();
}

void CObject::Serialize(creg::ISerializer *s)
{
	if (s->IsWriting ()) {
		int size=0;
		for (std::list<CObject*>::iterator i=listening.begin();i!=listening.end();++i) {
			if ((*i)->GetClass()!=CObject::StaticClass())
				size++;
		}
		s->Serialize (&size, sizeof(int));
		for (std::list<CObject*>::iterator i=listening.begin();i!=listening.end();++i) {
			if ((*i)->GetClass()!=CObject::StaticClass())
				s->SerializeObjectPtr((void **)&*i,(*i)->GetClass());
			else
				logOutput.Print("Death dependance not serialized in %s",this->GetClass()->name.c_str());
		}
	} else {
		int size;
		s->Serialize (&size, sizeof(int));
		for (int i=0;i<size;i++) {
			std::list<CObject*>::iterator itm=listening.insert(listening.end(),0);
			s->SerializeObjectPtr((void **)&*itm,0);
		}
	}
}

void CObject::PostLoad()
{
	for (std::list<CObject*>::iterator i=listening.begin();i!=listening.end();i++) {
		m_setOwner(__FILE__, __LINE__, __FUNCTION__);
		(*i)->listeners.insert((*i)->listeners.end(),this);
	}
	m_resetGlobals();
}

void CObject::DependentDied(CObject* o)
{
}

void CObject::AddDeathDependence(CObject *o)
{
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	o->listeners.insert(o->listeners.end(),this);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	listening.insert(listening.end(),o);
	m_resetGlobals();
}

void CObject::DeleteDeathDependence(CObject *o)
{
	//note that we can be listening to a single object from several different places (like curreclaim in CBuilder and lastAttacker in CUnit, grr) so we should only remove one of them
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	ListErase<CObject*>(listening, o);
	m_setOwner(__FILE__, __LINE__, __FUNCTION__);
	ListErase<CObject*>(o->listeners, this);
	m_resetGlobals();
}
