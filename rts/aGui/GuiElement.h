#ifndef GUIELEMENT_H
#define GUIELEMENT_H

#include <list>
#include <SDL_events.h>

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
	
	virtual void AddChild(GuiElement* elem)
	{
		children.push_back(elem);
	};
	
	void SetPos(float x, float y);
	void SetSize(float x, float y);

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

#endif