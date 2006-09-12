// GUItable.cpp: implementation of the GUItable class.
//
//////////////////////////////////////////////////////////////////////

#include "GUItable.h"
#include "GUIscrollbar.h"

#include "GUIfont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define KNOB_SIZE 16

GUItable::GUItable(int x1, int y1, int w1, int h1, const vector<string>* contents, Functor3<GUItable*, int,int>clicked):GUIpane(x1,y1,w1,h1), click(clicked), data()
{
	rowHeight=(int)guifont->GetHeight();
	length=data.size()*rowHeight;
	position=0;
	selected=0;
	numLines=h/rowHeight-1;
	
	width=w1;

	ChangeList(contents);
	name="table";
}

GUItable::~GUItable()
{
}

int GUItable::GetSelected() const
{
	return selected;
}

void GUItable::SetSelect(int i,int button)
{
	if(i<numEntries&&i>=0)
	{
		selected=i;
	}
}

void GUItable::Select(int i,int button)
{
	if(i<numEntries&&i>=0)
	{
		selected=i;
		click(this, selected,button);
	}
}


void GUItable::PrivateDraw()
{
	glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f, 0.0f, 0.0f, GUI_TRANS);	
	glBegin(GL_QUADS);		
		glTexCoord2d(0.0f, 0.0f);
		glVertex3f(10, 10, 0);
		glTexCoord2d(0.0f, 1.0f);		
		glVertex3f(10, h-10, 0);		
		glTexCoord2d(1.0f, 1.0f);		
		glVertex3d(w-10, h-10, 0);
		glTexCoord2d(1.0f, 0.0f);		
		glVertex3f(w-10, 10, 0);		
	glEnd();
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glCallList(displayList);
	
	int inSet=16;
	
	if(w<200)
		inSet=0;

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	
	if(!header.empty())
	{
		int size=header.size();
		float headerPos=0;

		for(int i=0; i<size; i++)
		{
			header[i].position=headerPos*w+inSet;
			guifont->PrintColor(header[i].position, 10, header[i].title);

			headerPos+=header[i].size;
		}
		
		glBindTexture(GL_TEXTURE_2D, 0);
		glBegin(GL_LINES);
			glVertex3f(max(4.f, inSet/2.0f), 10+rowHeight, 0);
			glVertex3f(width-max(4.f, inSet/2.0f), 10+rowHeight, 0);		
		glEnd();

		for(int i=0; i<numLines && (i+position)<numEntries; i++)
		{
			if((i+position)==selected)
				glColor4f(1.0f,0.4f,0.4f,1.0f);

			string line=data[i+position];
			size_t tabPos=line.find("\t");
			int j=0;

			while(tabPos!=string::npos)
			{
				string contents=line.substr(0, tabPos);

				guifont->Print(header[j].position, (i+2)*rowHeight, contents);

				line=line.substr(tabPos+1, string::npos);
				tabPos=line.find("\t");
				j++;
			}
			guifont->Print(header[j].position, (i+2)*rowHeight, line.substr(0, tabPos));

			glColor4f(1.0f,1.0f,1.0f,1.0f);

		}
	}
	else
	{
		for(int i=0; i<numLines && (i+position)<numEntries; i++)
		{
			if((i+position)==selected)
				glColor4f(1.0f,0.4f,0.4f,1.0f);

			guifont->Print(inSet, (i+1)*rowHeight, data[i+position]);			
			glColor4f(1.0f,1.0f,1.0f,1.0f);
		}
	}
}

bool GUItable::MouseDownAction(int x, int y, int button)
{
	if(!LocalInside(x, y)||button==2)
		return false;
	return true;		
}

bool GUItable::MouseUpAction(int x, int y, int button)
{
	int i;

	if(!LocalInside(x, y)||button==2)
		return false;

	if(x>width) return false;

	if(!header.empty())
		i=(y/rowHeight)+position-2;
	else
		i=(y/rowHeight)+position-1;

	Select(i,button);
	
	return true;
}

void GUItable::Scrolled(int pos)
{
	if(pos>=0)
		position=pos;
}
	
void GUItable::ChangeList(const vector<string>* contents)
{
	// we kill of the scroll bar
	if(bar)
		RemoveChild(bar);

	data.clear();

	// add the new contents
	if(contents)
		data.insert(data.begin(), contents->begin(), contents->end());

	numEntries = data.size();

	width=w;

	if(numEntries>numLines)
	{
		width=w-KNOB_SIZE;	
		// and now we add a new one with the right proportions
		bar=new GUIscrollbar(width, 0, h, numEntries-numLines+1, makeFunctor((Functor1<int> *)0, *this, &GUItable::Scrolled));
		AddChild(bar);
	}
}


void GUItable::SetHeader(const vector<HeaderInfo>& newHeaders)
{
	header.clear();
	header.insert(header.begin(), newHeaders.begin(), newHeaders.end());

	if(!header.empty())	
		numLines=h/rowHeight-2;
	else
		numLines=h/rowHeight-1;
}

