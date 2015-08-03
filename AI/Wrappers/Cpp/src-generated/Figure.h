/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_FIGURE_H
#define _CPPWRAPPER_FIGURE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Figure {

public:
	virtual ~Figure(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * @brief Creates a cubic Bezier spline figure
	 * Creates a cubic Bezier spline figure from pos1 to pos4,
	 * with control points pos2 and pos3.
	 * 
	 * - Each figure is part of a figure group
	 * - When creating figures, use 0 as \<figureGroupId\> to create
	 *   a new figure group.
	 *   The id of this figure group is returned in \<ret_newFigureGroupId\>
	 * - \<lifeTime\> specifies how many frames a figure should live
	 *   before being auto-removed; 0 means no removal
	 * - \<arrow\> == true means that the figure will get an arrow at the end
	 * @param arrow  true: means that the figure will get an arrow at the end
	 * @param lifeTime  how many frames a figure should live before being autoremoved, 0 means no removal
	 * @param figureGroupId  use 0 to get a new group
	 * @param ret_newFigureGroupId  the new group
	 */
public:
	virtual int DrawSpline(const springai::AIFloat3& pos1, const springai::AIFloat3& pos2, const springai::AIFloat3& pos3, const springai::AIFloat3& pos4, float width, bool arrow, int lifeTime, int figureGroupId) = 0;

	/**
	 * @brief Creates a straight line
	 * Creates a straight line from pos1 to pos2.
	 * 
	 * - Each figure is part of a figure group
	 * - When creating figures, use 0 as \<figureGroupId\> to create a new figure group.
	 *   The id of this figure group is returned in \<ret_newFigureGroupId\>
	 * @param lifeTime specifies how many frames a figure should live before being auto-removed;
	 *                 0 means no removal
	 * @param arrow true means that the figure will get an arrow at the end
	 * @param arrow  true: means that the figure will get an arrow at the end
	 * @param lifeTime  how many frames a figure should live before being autoremoved, 0 means no removal
	 * @param figureGroupId  use 0 to get a new group
	 * @param ret_newFigureGroupId  the new group
	 */
public:
	virtual int DrawLine(const springai::AIFloat3& pos1, const springai::AIFloat3& pos2, float width, bool arrow, int lifeTime, int figureGroupId) = 0;

	/**
	 * Sets the color used to draw all lines of figures in a figure group.
	 * @param color_colorS3  (x, y, z) -> (red, green, blue)
	 */
public:
	virtual void SetColor(int figureGroupId, const springai::AIColor& color, short alpha) = 0;

	/**
	 * Removes a figure group, which means it will not be drawn anymore.
	 */
public:
	virtual void Remove(int figureGroupId) = 0;

}; // class Figure

}  // namespace springai

#endif // _CPPWRAPPER_FIGURE_H

