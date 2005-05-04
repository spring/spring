#include "GUIswitchBar.h"
#include <string>
#include "GUIbutton.h"

const int switchBarSize=20;
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
GUIswitchBar::GUIswitchBar():GUIframe(-gu->screenx, -gu->screeny, gu->screenx, gu->screeny)
{
	name="switchbar";
	embedded=NULL;
}

GUIswitchBar::~GUIswitchBar()
{
}

void GUIswitchBar::AddSwitchableChild(GUIframe *child, const string& caption1)
{

	GUIswitchableCaption *capt=dynamic_cast<GUIswitchableCaption*>(child);
	
	string caption;
	
	if(capt)
		caption=capt->GetCaption();
	else 
		caption=caption1;

	
	if(caption=="embedded")
	{
		embedded=child;
		AddChild(child);
		return;
	}
		
	GUIbutton *button=new GUIbutton(0, 0, caption, makeFunctor((Functor1<GUIbutton*>*) 0, *this, &GUIswitchBar::SwitchClicked));


	buttons[button]=child;
	AddChild(child);
	AddChild(button);

	child->x-=x;
	child->y-=y;

	LayoutButtons();
}

void GUIswitchBar::RemoveChild(GUIframe *item)
{
	map<GUIbutton*, GUIframe*>::iterator i=buttons.begin();
	map<GUIbutton*, GUIframe*>::iterator e=buttons.end();

	for(;i!=e; i++)
	{
		if(item==i->second)
		{
			GUIframe::RemoveChild(i->first);
			buttons.erase(i);
			// BUG: compiler thinks erase returns void.
			// as a workaround: begin anew.
			i=buttons.begin();
		}
	}
	GUIframe::RemoveChild(item);
	if(item==embedded)
		embedded=NULL;
}


void GUIswitchBar::LayoutButtons()
{
	map<GUIbutton*, GUIframe*>::iterator i=buttons.begin();
	map<GUIbutton*, GUIframe*>::iterator e=buttons.end();

	int xoff=0;
	int h=i->first->h;
	for(;i!=e; i++)
	{
		GUIbutton *b=i->first;
		b->x=xoff-x;
		b->y=gu->screeny-b->h-y;
		xoff+=b->w+5;
	}
	if(embedded)
	{
		embedded->x=xoff-x;
		embedded->y=gu->screeny-h-y;
		embedded->w=gu->screenx-xoff;
		embedded->h=h;
		embedded->SizeChanged();
	}
}

void GUIswitchBar::SwitchClicked(GUIbutton* button)
{
	GUIframe *f=buttons[button];

	f->ToggleHidden();

	f->Raise();
}