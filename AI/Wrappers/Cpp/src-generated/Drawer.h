/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_DRAWER_H
#define _CPPWRAPPER_DRAWER_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Drawer {

public:
	virtual ~Drawer(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * @param pos_posF3  on this position, only x and z matter
	 */
public:
	virtual void AddNotification(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha) = 0;

	/**
	 * @param pos_posF3  on this position, only x and z matter
	 * @param label  create this text on pos in my team color
	 */
public:
	virtual void AddPoint(const springai::AIFloat3& pos, const char* label) = 0;

	/**
	 * @param pos_posF3  remove map points and lines near this point (100 distance)
	 */
public:
	virtual void DeletePointsAndLines(const springai::AIFloat3& pos) = 0;

	/**
	 * @param posFrom_posF3  draw line from this pos
	 * @param posTo_posF3  to this pos, again only x and z matter
	 */
public:
	virtual void AddLine(const springai::AIFloat3& posFrom, const springai::AIFloat3& posTo) = 0;

	/**
	 * This function allows you to draw units onto the map.
	 * - they only show up on the local player's screen
	 * - they will be drawn in the "standard pose" (as if before any COB scripts are run)
	 * @param rotation  in radians
	 * @param lifeTime  specifies how many frames a figure should live before being auto-removed; 0 means no removal
	 * @param teamId  teamId affects the color of the unit
	 */
public:
	virtual void DrawUnit(UnitDef* toDrawUnitDef, const springai::AIFloat3& pos, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing) = 0;

public:
	virtual int TraceRay(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags) = 0;

public:
	virtual int TraceRayFeature(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags) = 0;

public:
	virtual springai::Figure* GetFigure() = 0;

public:
	virtual springai::PathDrawer* GetPathDrawer() = 0;

}; // class Drawer

}  // namespace springai

#endif // _CPPWRAPPER_DRAWER_H

