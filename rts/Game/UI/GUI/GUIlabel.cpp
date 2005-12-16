#include "GUIlabel.h"
#include <string>
#include "GUIfont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUIlabel::GUIlabel(const int x1, const int y1, int w1, int h1, const std::string& caption):GUIframe(x1,y1,w1, h1)
{
	displayList=glGenLists(1);
	SetCaption(caption);
}

GUIlabel::~GUIlabel()
{
	glDeleteLists(displayList, 1);
}

string& GUIlabel::GetCaption()
{
	return caption;
}

void GUIlabel::SetCaption(const string& c)
{
	caption=c;
	BuildList();
}


void GUIlabel::PrivateDraw()
{
	glCallList(displayList);
}

void GUIlabel::BuildList()
{
	glNewList(displayList, GL_COMPILE);
	
	glColor3f(1, 1, 1);

	size_t lf;
	int y=0;
	string temp=caption;
	while((lf=temp.find("\n"))!=string::npos)
	{
		guifont->PrintColor(0, y, temp.substr(0, lf));

		y+=(int)guifont->GetHeight();
		temp=temp.substr(lf+1, string::npos);		
	}
	guifont->PrintColor(0, y, temp);

	glEndList();
}

