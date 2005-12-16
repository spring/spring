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
	std::set<aiObject*>::iterator di;
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
	listening.erase(o);
	o->listeners.erase(this);
}
