#include "GUImatrix.h"

#include "GUIfont.h"



GUImatrix::GUImatrix(int x, int y, int w, int h)

: GUIframe(x, y, w, h)

{

	lines=0;

	columns=0;

}



GUImatrix::~GUImatrix(void)

{

}



void GUImatrix::PrivateDraw(void)

{

	for(int a=0;a<lines;++a){

		int y=a*17+20;

		int x=5;

		DrawLine(3,y-2,5+totalWidth,y-2);

		for(int b=0;b<columns;++b){

			guifont->Print(x, y, values[a][b]);

			x+=widths[b];

		}

	}

	DrawLine(3,lines*17+18,3+totalWidth,lines*17+18);



	int x=3;

	for(int b=0;b<columns;++b){

		DrawLine(x,18,x,lines*17+18);

		x+=widths[b];

	}

	DrawLine(x,18,x,lines*17+18);

}



void GUImatrix::SetValues(std::vector<std::vector<std::string> > values,std::vector<std::string> tooltips)

{

	this->values=values;

	this->tooltips=tooltips;



	lines=values.size();

	if(lines==0)

		return;



	columns=values[0].size();

	widths.resize(columns);



	totalWidth=0;

	for(int a=0;a<columns;++a){

		int max=0;

		for(int b=0;b<lines;++b){

			if(guifont->GetWidth(values[b][a])>max)

				max=guifont->GetWidth(values[b][a]);

		}

		widths[a]=max+5;

		totalWidth+=widths[a];

	}

}



void GUImatrix::DrawLine(int x1, int y1, int x2, int y2)

{

	glBindTexture(GL_TEXTURE_2D,0);

	glColor4f(1,1,1,0.5);

	glBegin(GL_LINES);

		glVertex2f(x1,y1);

		glVertex2f(x2,y2);

	glEnd();

	glColor4f(1,1,1,1);

}



std::string GUImatrix::Tooltip(void)

{

	return tooltip;

}



bool GUImatrix::MouseMoveAction(int x, int y, int xrel, int yrel, int button)
{
	tooltip="";
	if(y>18 && y<lines*17+18){
		int xp=3;
		for(int a=0;a<columns;++a){

			xp+=widths[a];

			if(x<xp){

				tooltip=tooltips[a];

				break;

			}

		}
	}
	return GUIframe::MouseMoveAction(x,y,xrel,yrel,button);
}
