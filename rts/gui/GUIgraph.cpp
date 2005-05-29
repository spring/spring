#include "GUIgraph.h"

#include "GUIfont.h"







GUIgraph::GUIgraph(int x, int y, int w, int h):GUIframe(x, y, w, h)

{

	displayList=glGenLists(1);



	showDif=false;

	timebase=1;

	BuildList();

}



GUIgraph::~GUIgraph()

{

	glDeleteLists(displayList, 1);

}





void GUIgraph::PrivateDraw()

{

	glCallList(displayList);

}



void GUIgraph::SetDatasets(const vector<GUIgraph::Dataset>& newData,bool showDif,int button,string name)

{

	this->showDif=showDif;

	graphNames[button]=name;

	data[button].clear();

	data[button].insert(data[button].begin(), newData.begin(), newData.end());

	maximum=0;

	

	for(int a=0;a<2;++a){

		vector<GUIgraph::Dataset>::iterator i=data[a].begin();

		vector<GUIgraph::Dataset>::iterator e=data[a].end();



		for(;i!=e; i++)

		{

			vector<float>::iterator j=i->points.begin();

			vector<float>::iterator k=i->points.end();

			float last=0;

		

			for(;j!=k; j++){

				if(*j-last>maximum)

					maximum=*j-last;

				if(showDif)

					last=*j;

			}

		}

	}

	

	BuildList();

}



void GUIgraph::SetTimebase(const float t)

{

	timebase=t;

	BuildList();

}



void GUIgraph::BuildList()

{

//	if(data[0].empty())

//		return;



	static GLuint graphPaper=0;



	if(!graphPaper)

		graphPaper=Texture("bitmaps/graphPaper.bmp");



	char buf[50];

	sprintf(buf, "%i", (int)maximum);



	int sideBar=(int)(guifont->GetWidth(buf))+10;

	int bottomBar=(int)(guifont->GetHeight())+10;

	w-=sideBar;

	//h-=bottomBar;

	

	int numTicks=(int)(h/(guifont->GetHeight()+40));

	

	glNewList(displayList, GL_COMPILE);

	

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	

	glLineWidth(2);

	



	glBindTexture(GL_TEXTURE_2D, graphPaper);



	glBegin(GL_QUADS);

		glTexCoord2f(0, numTicks);

		glVertex2f(0, 0);

		glTexCoord2f(0, 0);

		glVertex2f(0, h);

		glTexCoord2f(w/(float)h*numTicks, 0);

		glVertex2f(w, h);

		glTexCoord2f(w/(float)h*numTicks, numTicks);

		glVertex2f(w, 0);

	glEnd();



	vector<GUIgraph::Dataset>::iterator i=data[0].begin();

	vector<GUIgraph::Dataset>::iterator e=data[0].end();



	float y=10;

	float width=0;

	float height=data[0].size()*(guifont->GetHeight()+5);

	

	for(;i!=e; i++)

	{

		float tempWidth=guifont->GetWidth(i->caption);

		if(tempWidth>width) width=tempWidth;

	}

	if(guifont->GetWidth(graphNames[0])>width)

		width=guifont->GetWidth(graphNames[0]);

	if(guifont->GetWidth(graphNames[1])>width)

		width=guifont->GetWidth(graphNames[1]);



	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f(0,0,0, 0.4);



	glBegin(GL_QUADS);

		glVertex2f(w-width-15, y-5);

		glVertex2f(w-width-15, y+height-2);

		glVertex2f(w-5, y+height-2);

		glVertex2f(w-5, y-5);

	glEnd();

	

	glColor3f(1,1,1);

	

	i=data[0].begin();

	e=data[0].end();



	for(;i!=e; i++)

	{

		glColor3f(i->r, i->g, i->b);

		guifont->Print(w-width-10, y, i->caption);



		y+=guifont->GetHeight()+5;

	}



	for(int a=0;a<2;++a){

		i=data[a].begin();

		e=data[a].end();



		glColor3f(0, 0, 0);

		guifont->Print(w-width-10, y, graphNames[a]);



		if(a==1){

			glLineStipple(3,0x5555);

			glEnable(GL_LINE_STIPPLE);

		}

		glBindTexture(GL_TEXTURE_2D, 0);



		glBegin(GL_LINE_STRIP);

			glVertex2f(w-width-30, y+6);

			glVertex2f(w-width-15, y+6);

		glEnd();

		y+=guifont->GetHeight()+5;



		for(;i!=e; i++){

			glColor3f(i->r, i->g, i->b);



			glBegin(GL_LINE_STRIP);



			vector<float>::iterator j=i->points.begin();

			vector<float>::iterator k=i->points.end();

			float last=0;



			int n=0;

			int numPoints=i->points.size();



			for(;j!=k; j++)

			{

				float x=n/(float)numPoints*w;

				float y=h-((*j-last)/maximum*h);

				glVertex2f(x, y);

				n++;

				if(showDif)

					last=*j;

			}



			glEnd();



		}

		glDisable(GL_LINE_STIPPLE);

	}



	glColor3f(1,1,1);

	

	for(int i=0; i<=numTicks; i++)

	{

		int tick=(int)(maximum-i*maximum/numTicks);

		if(showDif)

			tick/=16;		//make it per sec



		sprintf(buf, "%i", tick);

	

		guifont->Print(w+5, i*h/numTicks-guifont->GetHeight()/2.0, buf);

	}



	

	w+=sideBar;

	//h+=bottomBar;

	

	glPopAttrib();



	glEndList();

	

}

