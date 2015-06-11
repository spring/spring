/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJECT_H
#define OBJECT_H

#include <deque>
#include <map>
#include "ObjectDependenceTypes.h"
#include "System/creg/creg_cond.h"


class CObject
{
	CR_DECLARE(CObject)

public:
	CObject();
	virtual ~CObject();
	void Serialize(creg::ISerializer* ser);
	virtual void Detach();

	/// Request to not inform this when obj dies
	virtual void DeleteDeathDependence(CObject* obj, DependenceType dep);
	/// Request to inform this when obj dies
	virtual void AddDeathDependence(CObject* obj, DependenceType dep);
	/// Called when an object died, that this is interested in
	virtual void DependentDied(CObject* obj);

public:
	typedef std::deque<CObject*> TSyncSafeSet;
	typedef std::map</*DependenceType*/ int, TSyncSafeSet> TDependenceMap; // we use a std::map here, so the deques only get created ondemand (their ctor/dtor is heavy!)

protected:
	const TSyncSafeSet& GetListeners(const DependenceType dep) { return listeners[dep]; }
	const TDependenceMap& GetAllListeners() const { return listeners; }
	const TSyncSafeSet& GetListening(const DependenceType dep)  { return listening[dep]; }
	const TDependenceMap& GetAllListening() const { return listening; }

protected:
	bool detached;
	TDependenceMap listeners;
	TDependenceMap listening;
};




#endif /* OBJECT_H */
