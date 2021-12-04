/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUIELEMENT_H
#define GUIELEMENT_H

#include <list>
#include <SDL_events.h>
#include "Rendering/GL/VertexArrayTypes.h"

struct VAO;
class VBO;

namespace agui
{

class GuiElement
{
public:
	GuiElement(GuiElement* parent = nullptr);
	virtual ~GuiElement();

	void Draw();
	void DrawBox(unsigned int idx = 0);
	void DrawOutline();

	bool HandleEvent(const SDL_Event& ev);
	bool MouseOver(int x, int y) const;
	bool MouseOver(float x, float y) const;

	#if 0
	VAO* GetVAO(unsigned int k);
	VBO* GetVBO(unsigned int k);
	#endif

	static void UpdateDisplayGeo(int x, int y, int offsetX, int offsetY);
	static float PixelToGlX(int x);
	static float PixelToGlY(int y);
	static float GlToPixelX(float x);
	static float GlToPixelY(float y);

	virtual void AddChild(GuiElement* elem);

	void SetPos(float x, float y);
	void SetSize(float x, float y, bool fixed = false);
	void SetDepth(float z) { depth = z; }

	virtual bool HasTexCoors() const { return false; }
	bool SizeFixed() const { return fixedSize; }

	const float* GetSize() const { return size; }
	const float* GetPos() const { return pos; }
	float GetDepth() const { return depth; }
	void SetWeight(unsigned newWeight) { weight = newWeight; }
	unsigned Weight() const { return weight; }

	void GeometryChange();

	float GetMidY() const { return (pos[1] + (size[1] * 0.5f)); }
	float DefaultOpacity() const { return 0.8f; }

	virtual float Opacity() const {
		if (parent != nullptr)
			return parent->Opacity();

		return DefaultOpacity();
	};

	void Move(float x, float y);

	GuiElement* GetRoot() {
		if (parent != nullptr)
			return parent->GetRoot();

		return this;
	};

protected:
	GuiElement* parent;

	typedef std::list<GuiElement*> ChildList;
	ChildList children;

	virtual void DrawSelf() {}
	virtual bool HandleEventSelf(const SDL_Event& ev) { return false; }
	virtual void GeometryChangeSelf();

	void GeometryChangeSelfRaw(unsigned int bufIndx, unsigned int numBytes, const VA_TYPE_T* elemsPtr) const;

	static int screensize[2];
	static int screenoffset[2];

	bool fixedSize;

	float pos[2]; // top-left x,y coor
	float size[2]; // x,y dimensions
	float depth = 0.0f;

	unsigned weight;
	unsigned elIndex;
};

}

#endif
