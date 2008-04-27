#include "StdAfx.h"
// Object.cpp: implementation of the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
#include "mmgr.h"
#include "creg/STL_Set.h"
#include "LogOutput.h"

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
		(*di)->DependentDied(this);
		ListErase<CObject*>((*di)->listening, this);
	}
	for(di=listening.begin();di!=listening.end();++di){
		ListErase<CObject*>((*di)->listeners, this);
	}
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
	for (std::list<CObject*>::iterator i=listening.begin();i!=listening.end();i++)
		(*i)->listeners.insert((*i)->listeners.end(),this);
}

void CObject::DependentDied(CObject* o)
{
//	MessageBox(0,"wrong","ag",0);
}

void CObject::AddDeathDependence(CObject *o)
{
	o->listeners.insert(o->listeners.end(),this);
	listening.insert(listening.end(),o);
}

void CObject::DeleteDeathDependence(CObject *o)
{
	//note that we can be listening to a single object from several different places (like curreclaim in CBuilder and lastAttacker in CUnit, grr) so we should only remove one of them
	ListErase<CObject*>(listening, o);
	ListErase<CObject*>(o->listeners, this);
}
