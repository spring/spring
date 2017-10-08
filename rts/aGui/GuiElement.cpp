/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // memset
#include "GuiElement.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VertexArrayTypes.h"

namespace agui
{

static std::vector<VBO> quadBuffers[2];
static std::vector<size_t> freeIndices;




int GuiElement::screensize[2];
int GuiElement::screenoffset[2];

GuiElement::GuiElement(GuiElement* _parent) : parent(_parent), fixedSize(false), weight(1), vboIndex(-1)
{
	if (quadBuffers[0].empty()) {
		quadBuffers[0].reserve(16);
		quadBuffers[1].reserve(16);
	}

	if (freeIndices.empty()) {
		vboIndex = quadBuffers[0].size();

		quadBuffers[0].emplace_back();
		quadBuffers[1].emplace_back();
	} else {
		vboIndex = freeIndices.back();
		freeIndices.pop_back();
	}

	quadBuffers[0][vboIndex].Generate();
	quadBuffers[1][vboIndex].Generate();

	std::memset(pos, 0, sizeof(pos));
	std::memset(size, 0, sizeof(size));


	GeometryChangeSelf();

	if (parent != nullptr)
		parent->AddChild(this);

}

GuiElement::~GuiElement()
{
	for (auto i = children.begin(), e = children.end(); i != e; ++i) {
		delete *i;
	}

	quadBuffers[0][vboIndex].Delete();
	quadBuffers[1][vboIndex].Delete();
	quadBuffers[0][vboIndex] = std::move(VBO());
	quadBuffers[1][vboIndex] = std::move(VBO());
	freeIndices.push_back(vboIndex);
}



VBO* GuiElement::GetVBO(unsigned int k) { return &quadBuffers[k][vboIndex]; }

void GuiElement::DrawBox(unsigned int mode, unsigned int indx) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);


	VBO* vbo = &quadBuffers[indx][vboIndex];
	vbo->Bind(GL_ARRAY_BUFFER);

	glVertexPointer(2, GL_FLOAT, sizeof(VA_TYPE_2dT), nullptr);

	// texcoors are effectively ignored for all elements except Picture
	// (however, shader always samples from the texture so do not leave
	// them undefined)
	if (true || HasTexCoors()) {
		glClientActiveTexture(GL_TEXTURE0);
		glTexCoordPointer(2, GL_FLOAT, sizeof(VA_TYPE_2dT), nullptr);
	}

	vbo->Unbind();

	glDrawArrays(mode, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void GuiElement::GeometryChangeSelf()
{
	VA_TYPE_2dT vaElems[4];

	vaElems[0].x = pos[0]          ; vaElems[0].y = pos[1]          ; // TL
	vaElems[1].x = pos[0]          ; vaElems[1].y = pos[1] + size[1]; // BL
	vaElems[2].x = pos[0] + size[0]; vaElems[2].y = pos[1] + size[1]; // BR
	vaElems[3].x = pos[0] + size[0]; vaElems[3].y = pos[1]          ; // TR

	vaElems[0].s = 0.0f; vaElems[0].t = 1.0f;
	vaElems[1].s = 0.0f; vaElems[1].t = 0.0f;
	vaElems[2].s = 1.0f; vaElems[2].t = 0.0f;
	vaElems[3].s = 1.0f; vaElems[3].t = 1.0f;

	quadBuffers[0][vboIndex].Bind();
	quadBuffers[0][vboIndex].New(sizeof(vaElems), GL_DYNAMIC_DRAW, vaElems);
	quadBuffers[0][vboIndex].Unbind();
}

void GuiElement::GeometryChange()
{
	GeometryChangeSelf();

	for (auto i = children.begin(), e = children.end(); i != e; ++i) {
		(*i)->GeometryChange();
	}
}



void GuiElement::Draw()
{
	DrawSelf();

	for (auto i = children.begin(), e = children.end(); i != e; ++i) {
		(*i)->Draw();
	}
}

bool GuiElement::HandleEvent(const SDL_Event& ev)
{
	if (HandleEventSelf(ev))
		return true;

	for (auto i = children.begin(), e = children.end(); i != e; ++i) {
		if ((*i)->HandleEvent(ev))
			return true;
	}

	return false;
}


bool GuiElement::MouseOver(int x, int y) const
{
	const float mouse[2] = {PixelToGlX(x), PixelToGlY(y)};
	return (mouse[0] >= pos[0] && mouse[0] <= pos[0]+size[0]) && (mouse[1] >= pos[1] && mouse[1] <= pos[1]+size[1]);
}

bool GuiElement::MouseOver(float x, float y) const
{
	return (x >= pos[0] && x <= pos[0] + size[0]) && (y >= pos[1] && y <= pos[1] + size[1]);
}


void GuiElement::UpdateDisplayGeo(int x, int y, int xOffset, int yOffset)
{
	screensize[0] = x;
	screensize[1] = y;
	screenoffset[0] = xOffset;
	screenoffset[1] = yOffset;
}


float GuiElement::PixelToGlX(int x) { return        float(x - screenoffset[0]) / float(screensize[0]); }
float GuiElement::PixelToGlY(int y) { return 1.0f - float(y - screenoffset[1]) / float(screensize[1]); }

float GuiElement::GlToPixelX(float x) { return (x * float(screensize[0]) + float(screenoffset[0])); }
float GuiElement::GlToPixelY(float y) { return (y * float(screensize[1]) + float(screenoffset[1])); }


void GuiElement::AddChild(GuiElement* elem)
{
	children.push_back(elem);

	elem->SetPos(pos[0], pos[1]);
	elem->SetSize(size[0], size[1]);

	GeometryChange();
}

void GuiElement::SetPos(float x, float y)
{
	pos[0] = x;
	pos[1] = y;

	GeometryChange();
}

void GuiElement::SetSize(float x, float y, bool fixed)
{
	size[0] = x;
	size[1] = y;
	fixedSize = fixed;

	GeometryChange();
}

void GuiElement::Move(float x, float y)
{
	pos[0] += x;
	pos[1] += y;

	GeometryChangeSelf();

	for (auto i = children.begin(), e = children.end(); i != e; ++i) {
		(*i)->Move(x, y);
	}
}

}
