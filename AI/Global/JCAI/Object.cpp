//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "BaseAIObjects.h"

aiObject::~aiObject()
{
	std::multiset<aiObject*>::iterator di;
	for(di=listeners.begin();di!=listeners.end();++di){
		(*di)->DependentDied(this);
		(*di)->listening.erase(this);
	}
	for(di=listening.begin();di!=listening.end();++di){
		(*di)->listeners.erase(this);
	}
}

void aiObject::DependentDied(aiObject* o) { }

void aiObject::AddDeathDependence(aiObject *o)
{
	o->listeners.insert(this);
	listening.insert(o);
}

void aiObject::DeleteDeathDependence(aiObject *o)
{
	//note that we can be listening to a single object from several different places (like curreclaim in CBuilder and lastAttacker in CUnit, grr) so we should only remove one of them
	std::multiset<aiObject*>::iterator oi=listening.find(o);
	if(oi!=listening.end())
		listening.erase(oi);
	std::multiset<aiObject*>::iterator oi2=o->listeners.find(this);
	if(oi2!=o->listeners.end())
		o->listeners.erase(oi2);
}
