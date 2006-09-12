// GUIstateButton.cpp: implementation of the GUIstateButton class.
//
//////////////////////////////////////////////////////////////////////

#include "GUIstateButton.h"
#include "GUIfont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GUIstateButton::GUIstateButton(int x, int y, int w, vector<string>& states1, Functor2<GUIstateButton*, int> callback): GUIbutton(x, y, states1[0], (Functor1<GUIbutton*>)0), click(callback), states(states1)
{
	SetSize(w, 0);
	state=0;
	centerText=false;
}

GUIstateButton::~GUIstateButton()
{

}

void GUIstateButton::PrivateDraw()
{
	static GLuint tex=0;

	// draw button

	GUIbutton::PrivateDraw();

	// draw bips

	glPushAttrib(GL_CURRENT_BIT);

	if(!tex)
		tex=Texture("bitmaps/check.bmp");

	if(isInside)
		glColor4f(0.7f,0.7f,0.7f,1.0f);
	else
		glColor4f(1.0f,1.0f,1.0f,1.0f);	

	int s=w-7*states.size()-1;
	int t=0;

	glBindTexture(GL_TEXTURE_2D, tex);

	glEnable(GL_TEXTURE_2D);

	for(int i=0; i<states.size(); i++)
	{
		// box
		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2d(s, t);
			glTexCoord2f(0, 1);
			glVertex2d(s, t+h);
			glTexCoord2f(0.5f, 1);
			glVertex2d(s+8, t+h);				
			glTexCoord2f(0.5f, 0);
			glVertex2d(s+8, t);
		glEnd();

		// light
		if(i==state)
		{
			glColor3fv(color);
			glBegin(GL_QUADS);
				glTexCoord2f(0.5f, 0);
				glVertex2d(s, t);

				glTexCoord2f(0.5f, 1);
				glVertex2d(s, t+h);
	
				glTexCoord2f(1, 1);
				glVertex2d(s+8, t+h);

				glTexCoord2f(1, 0);
				glVertex2d(s+8, t);				
			glEnd();

			if(isInside)
				glColor4f(0.7f,0.7f,0.7f,1.0f);
			else
				glColor4f(1.0f,1.0f,1.0f,1.0f);
		}
		
		s+=7;
	}

	glPopAttrib();
}

void GUIstateButton::Action()
{
	state++;
	if(state>=states.size()) state=0;
	
	SetCaption(states[state]);
	
	click(this, state);
}

void GUIstateButton::BuildList()
{
	static GLuint tex=0;
	if(!tex)
	{
		vector < paletteentry_s > vTransparentColors;
		paletteentry_s peBlack;
		peBlack.peBlue = peBlack.peGreen = peBlack.peRed = 0;
		peBlack.peFlags = 0;
		vTransparentColors.push_back( peBlack );
		tex=Texture("bitmaps/stateButton.bmp", &vTransparentColors);
	}

	glNewList(displayList, GL_COMPILE);

	glBindTexture(GL_TEXTURE_2D, tex);

	DrawThemeRect(10, 32, w, h);


	size_t pipe=caption.find("|");

	color[0]=1.0f;
	color[1]=1.0f;
	color[2]=1.0f;

	if(pipe!=string::npos)
	{
		string colname=caption.substr(pipe+1, string::npos);
		if(colname=="red")
		{
			color[0]=1.0f;
			color[1]=0.0f;
			color[2]=0.0f;
		}
		else if(colname=="green")
		{
			color[0]=0.0f;
			color[1]=1.0f;
			color[2]=0.0f;
		}
		else if(colname=="blue")
		{
			color[0]=0.0f;
			color[1]=0.0f;
			color[2]=1.0f;
		}
		guifont->Print(7, (h-guifont->GetHeight())/2.0f+1, caption.substr(0, pipe));
	}
	else
		guifont->output(7, (h-guifont->GetHeight())/2.0f+1, caption.c_str());

	glEndList();
}

void GUIstateButton::SetState(int s)
{
	state=s;
	if(state>=states.size()) state=0;
	SetCaption(states[state]);	
	BuildList();
}

void GUIstateButton::SetStates(const vector<string>& s)
{
	states=s;
	SetState(state);
}


