/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EXPLOSION_LISTENER_H
#define _EXPLOSION_LISTENER_H

#include <vector>


struct WeaponDef;
struct CExplosionParams;


class IExplosionListener
{
public:
	/**
	 * Informs listeners about an explosion that has occured.
	 * @see EventClient#Explosion
	 */
	virtual void ExplosionOccurred(const CExplosionParams& event) = 0;
protected:
	~IExplosionListener();
};


/**
 * Base
 */
class CExplosionCreator
{
public:
	static void AddExplosionListener(IExplosionListener* listener);
	static void RemoveExplosionListener(IExplosionListener* listener);

	/**
	 * Sends the event to all registered listeners.
	 */
	static void FireExplosionEvent(const CExplosionParams& event);

private:
	static std::vector<IExplosionListener*> explosionListeners;
};

#endif /* _EXPLOSION_LISTENER_H */
