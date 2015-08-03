/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPDRAWER_H
#define _CPPWRAPPER_WRAPPDRAWER_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Drawer.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappDrawer : public Drawer {

private:
	int skirmishAIId;

	WrappDrawer(int skirmishAIId);
	virtual ~WrappDrawer();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Drawer* GetInstance(int skirmishAIId);

	/**
	 * @param pos_posF3  on this position, only x and z matter
	 */
public:
	// @Override
	virtual void AddNotification(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha);

	/**
	 * @param pos_posF3  on this position, only x and z matter
	 * @param label  create this text on pos in my team color
	 */
public:
	// @Override
	virtual void AddPoint(const springai::AIFloat3& pos, const char* label);

	/**
	 * @param pos_posF3  remove map points and lines near this point (100 distance)
	 */
public:
	// @Override
	virtual void DeletePointsAndLines(const springai::AIFloat3& pos);

	/**
	 * @param posFrom_posF3  draw line from this pos
	 * @param posTo_posF3  to this pos, again only x and z matter
	 */
public:
	// @Override
	virtual void AddLine(const springai::AIFloat3& posFrom, const springai::AIFloat3& posTo);

	/**
	 * This function allows you to draw units onto the map.
	 * - they only show up on the local player's screen
	 * - they will be drawn in the "standard pose" (as if before any COB scripts are run)
	 * @param rotation  in radians
	 * @param lifeTime  specifies how many frames a figure should live before being auto-removed; 0 means no removal
	 * @param teamId  teamId affects the color of the unit
	 */
public:
	// @Override
	virtual void DrawUnit(UnitDef* toDrawUnitDef, const springai::AIFloat3& pos, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing);

public:
	// @Override
	virtual int TraceRay(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags);

public:
	// @Override
	virtual int TraceRayFeature(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags);

public:
	// @Override
	virtual springai::Figure* GetFigure();

public:
	// @Override
	virtual springai::PathDrawer* GetPathDrawer();
}; // class WrappDrawer

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPDRAWER_H

