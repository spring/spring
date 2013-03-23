/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExplosionListener.h"


std::set<IExplosionListener*> CExplosionCreator::explosionListeners;


IExplosionListener::~IExplosionListener()
{
	CExplosionCreator::RemoveExplosionListener(this);
}

void CExplosionCreator::AddExplosionListener(IExplosionListener* listener)
{
	explosionListeners.insert(listener);
}

void CExplosionCreator::RemoveExplosionListener(IExplosionListener* listener)
{
	explosionListeners.erase(listener);
}

void CExplosionCreator::FireExplosionEvent(const CExplosionEvent& event)
{
	std::set<IExplosionListener*>::const_iterator expList;
	for (expList = explosionListeners.begin(); expList != explosionListeners.end(); ++expList) {
		(*expList)->ExplosionOccurred(event);
	}
}

