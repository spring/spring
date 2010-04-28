/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LAYOUT_H
#define LAYOUT_H

class Layout
{
public:
	Layout()
	{
		itemSpacing = 0.005f;
		borderSpacing = 0.005f;
		borderWidth = 0.0f;
	};
	
	void SetBorder(float thickness)
	{
		borderWidth = thickness;
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
	float borderSpacing;
	float itemSpacing;
	float borderWidth;
};

#endif
