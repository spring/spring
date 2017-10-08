/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PICTURE_H
#define PICTURE_H

#include <string>

#include "GuiElement.h"

namespace agui
{

class Picture : public GuiElement
{
public:
	Picture(GuiElement* parent = nullptr);
	~Picture();

	void Load(const std::string& file);

	bool HasTexCoors() const override { return true; }

private:
	void DrawSelf() override;
	
	unsigned texture;
	std::string file;
};

}

#endif
