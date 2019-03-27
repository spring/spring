/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LAYOUT_H
#define LAYOUT_H

class Layout
{
public:
	void SetBorder(bool visible)
	{
		visibleBorder = visible;
	};
	void SetBorderSpacing(float width)
	{
		borderSpacing = width;
	};
	void SetItemSpacing(float width)
	{
		itemSpacing = width;
	};

protected:
	float borderSpacing = 0.005f;
	float itemSpacing = 0.005f;
	bool visibleBorder = false;
};

#endif
