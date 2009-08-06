#ifndef GUIELEMENT_H
#define GUIELEMENT_H

#include <list>
#include <SDL_events.h>

namespace agui
{

class GuiElement
{
public:
	GuiElement(GuiElement* parent = NULL);
	virtual ~GuiElement();
	
	void Draw();
	bool HandleEvent(const SDL_Event& ev);
	bool MouseOver(int x, int y) const;
	bool MouseOver(float x, float y) const;
	
	static void UpdateDisplayGeo(int x, int y);
	static float PixelToGlX(int x);
	static float PixelToGlY(int y);
	
	virtual void AddChild(GuiElement* elem);
	
	void SetPos(float x, float y);
	void SetSize(float x, float y, bool fixed = false);
	bool SizeFixed() const
	{
		return fixedSize;
	};
	float* GetSize()
	{
		return size;
	};
	
	void GeometryChange();

	float GetMidY() const
	{
		return pos[1] + (size[1] / 2.0f);
	};

	float DefaultOpacity() const;
	virtual float Opacity() const
	{
		if (parent)
			return parent->Opacity();
		else
			return DefaultOpacity();
	};

	void Move(float x, float y);
	GuiElement* GetRoot()
	{
		if (parent)
			return parent->GetRoot();
		else
			return this;
	};

	void DrawBox(int how);

protected:
	GuiElement* parent;
	
	typedef std::list<GuiElement*> ChildList;
	ChildList children;
	
	virtual void DrawSelf() {};
	virtual bool HandleEventSelf(const SDL_Event& ev)
	{
		return false;
	};
	virtual void GeometryChangeSelf() {};
	
	static int screensize[2];
	
	bool fixedSize;
	float pos[2];
	float size[2];
};

}

#endif