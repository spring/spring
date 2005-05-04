#pragma once

#include "GUIframe.h"
#include <vector>
#include <string>

class GUImatrix : public GUIframe
{
public:
	GUImatrix(int x, int y, int w, int h);
	~GUImatrix(void);
	void PrivateDraw(void);

	int lines;
	int columns;
	int totalWidth;

	std::vector<int> widths;
	std::vector<std::string> tooltips;
	std::vector<std::vector<std::string> > values;
	void SetValues(std::vector<std::vector<std::string> > values,std::vector<std::string> tooltips);
	void DrawLine(int x1, int y1, int x2, int y2);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int button);
	std::string Tooltip(void);
};
