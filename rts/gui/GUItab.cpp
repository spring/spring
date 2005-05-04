#include "GUItab.h"
#include <string>
#include "GUIbutton.h"


const int tabSize=20;

using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUItab::GUItab(int x, int y, int w, int h):GUIframe(x, y, w, h)
{
}

GUItab::~GUItab()
{
}



void GUItab::AddChild(GUIframe *child)
{
	GUIbutton *button=new GUIbutton(0, 0, "", makeFunctor((Functor1<GUIbutton*>*) 0, *this, &GUItab::SwitchClicked));

	// FIXME: using the identifier as the title means this is
	// not localizeable.
	button->SetCaption(child->Identifier());
	
	button->SetTooltip(child->Tooltip());

	buttons[button]=child;
	order.push_back(button);
	GUIframe::AddChild(child);
	GUIframe::AddChild(button);

	LayoutButtons();

	SwitchClicked(button);
}

void GUItab::RemoveChild(GUIframe *item)
{
	vector<GUIbutton*>::iterator i=order.begin();
	vector<GUIbutton*>::iterator e=order.end();

	for(;i!=e;i++)
	{
		if((*i)==item)
		{
			buttons.erase(*i);
			order.erase(i);
			GUIframe::RemoveChild(item);
			return;
		}
	}
	GUIframe::RemoveChild(item);
}

void GUItab::LayoutButtons()
{
	vector<GUIbutton*>::iterator i=order.begin();
	vector<GUIbutton*>::iterator e=order.end();

	int xoff=0;
	int width=w/buttons.size()-5;
	for(;i!=e; i++)
	{
		GUIbutton *b=*i;
		b->x=xoff;
		b->y=2;
		b->SetSize(width, 0);
		xoff+=width+5;
	}
}

void GUItab::SwitchClicked(GUIbutton* button)
{
	GUIframe *f=buttons[button];
	
	map<GUIbutton*, GUIframe*>::iterator i=buttons.begin();
	map<GUIbutton*, GUIframe*>::iterator e=buttons.end();

	// hide all children
	for(;i!=e; i++)
	{
		i->first->SetHighlight(false);

		if(!i->second->isHidden())
			i->second->ToggleHidden();
	}
	// then unhide the selected one
	button->SetHighlight(true);
	f->ToggleHidden();
}

bool GUItab::EventAction(const std::string& event)
{
	if(event.compare(0, 6, "switch_"))
	{
		int num=atoi(event.substr(7, std::string::npos).c_str());

		if(num<order.size())
			SwitchClicked(order[num]);

		return true;
	}

	return GUIframe::EventAction(event);
}
