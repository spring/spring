/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // memset
#include "GuiElement.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VertexArrayTypes.h"

namespace agui
{

struct RenderElem {
public:
	RenderElem() = default;
	RenderElem(RenderElem&& e) { *this = std::move(e); }
	RenderElem(const RenderElem& e) = delete;

	RenderElem& operator = (const RenderElem& e) = delete;
	RenderElem& operator = (RenderElem&& e) {
		array = std::move(e.array);
		verts = std::move(e.verts);
		return *this;
	}

	void EnableAttribs() {
		// texcoors are effectively ignored for all elements except Picture
		// (however, shader always samples from the texture so do not leave
		// them undefined)
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_T), VA_TYPE_OFFSET(float, 0));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VA_TYPE_T), VA_TYPE_OFFSET(float, 3));
	}
	void DisableAttribs() {
	}

	void Generate() {
		array.Generate();
		verts.Generate();
	}
	void Bind() {
		array.Bind();
		verts.Bind();
	}
	void Unbind() {
		array.Unbind();
		verts.Unbind();
	}
	void Submit(unsigned int mode, unsigned int size = 4) {
		array.Bind();
		glDrawArrays(mode, 0, size);
		array.Unbind();
	}
	void Delete() {
		array.Delete();
		verts.Delete();
	}

	VAO array;
	VBO verts;
};

static std::vector<RenderElem> renderElems[3];
static std::vector<size_t> freeIndices;




int GuiElement::screensize[2];
int GuiElement::screenoffset[2];

GuiElement::GuiElement(GuiElement* _parent) : parent(_parent), fixedSize(false), weight(1), elIndex(-1)
{
	if (renderElems[0].empty()) {
		for (auto& rev: renderElems) {
			rev.reserve(16);
		}
	}

	if (freeIndices.empty()) {
		elIndex = renderElems[0].size();
		for (auto& rev: renderElems) {
			rev.emplace_back();
		}
	} else {
		elIndex = freeIndices.back();
		freeIndices.pop_back();
	}

	for (auto& rev: renderElems) {
		rev[elIndex].Generate();
	}

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
	for (auto& rev: renderElems) {
		rev[elIndex].Delete();
		rev[elIndex] = std::move(RenderElem());
	}

	freeIndices.push_back(elIndex);
}



void GuiElement::DrawBox(unsigned int indx) {
	renderElems[indx][elIndex].Submit(GL_TRIANGLE_STRIP, 4);
}

void GuiElement::DrawOutline() {
	renderElems[2][elIndex].Submit(GL_TRIANGLE_STRIP, 10);
}


void GuiElement::GeometryChangeSelfRaw(unsigned int bufIndx, unsigned int numBytes, const VA_TYPE_T* elemsPtr) const
{
	renderElems[bufIndx][elIndex].Bind();
	renderElems[bufIndx][elIndex].verts.New(numBytes, GL_DYNAMIC_DRAW, elemsPtr);
	renderElems[bufIndx][elIndex].EnableAttribs();
	renderElems[bufIndx][elIndex].Unbind();
	renderElems[bufIndx][elIndex].DisableAttribs();
}

void GuiElement::GeometryChangeSelf()
{
	{
		VA_TYPE_T vaElems[4];
		for (int i = 0; i < 4; ++i) {
			const bool bottom = (i & 1) == 0;
			const bool right = (i & 2) == 0;
			vaElems[i].p.x = pos[0] + right * size[0];
			vaElems[i].p.y = pos[1] + !bottom * size[1];
			vaElems[i].p.z = depth;
			vaElems[i].s = right * 1.0f;
			vaElems[i].t = bottom * 1.0f;
		}

		GeometryChangeSelfRaw(0, sizeof(vaElems), vaElems);
	}
	{
		const float outlineWidth = 1.0f;
		VA_TYPE_T vaElems[10];
		for (int i = 0; i < 10; ++i) {
			const bool outer = (i & 1) == 0;
			const bool bottom = (i & 4) == 0;
			const bool right = ((i + 2) & 4) == 0;
			vaElems[i].p.x = pos[0] + right * size[0] + ((outer ^ right) * 2 - 1) * outlineWidth / screensize[0];
			vaElems[i].p.y = pos[1] + !bottom * size[1] + ((outer ^ !bottom) * 2 - 1) * outlineWidth / screensize[1];
			vaElems[i].p.z = depth;
			vaElems[i].s = 0.0f;
			vaElems[i].t = 0.0f;
		}

		GeometryChangeSelfRaw(2, sizeof(vaElems), vaElems);
	}
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

	for (GuiElement* e: children) {
		e->Draw();
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
	return (mouse[0] >= pos[0] && mouse[0] <= pos[0] + size[0]) && (mouse[1] >= pos[1] && mouse[1] <= pos[1] + size[1]);
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
