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
	bool MouseOver(int x, int y);
	
	static void UpdateDisplayGeo(int x, int y);
	static float PixelToGlX(int x);
	static float PixelToGlY(int y);
	
	virtual void AddChild(GuiElement* elem);
	
	virtual void SetPos(float x, float y);
	virtual void SetSize(float x, float y);

	void Move(float x, float y);
	GuiElement* GetRoot()
	{
		if (parent)
			return parent->GetRoot();
		else
			return this;
	};

protected:
	GuiElement* parent;
	
	typedef std::list<GuiElement*> ChildList;
	ChildList children;
	
	void DrawBox(int how);
	virtual void DrawSelf() {};
	virtual bool HandleEventSelf(const SDL_Event& ev)
	{
		return false;
	};
	
	static int screensize[2];
	float pos[2];
	float size[2];
};

}

#endif