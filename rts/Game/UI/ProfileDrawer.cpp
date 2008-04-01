#include "ProfileDrawer.h"

#include <assert.h>

#include "TimeProfiler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/VertexArray.h"

ProfileDrawer* ProfileDrawer::instance = NULL;

void ProfileDrawer::Enable()
{
	assert(instance == 0);
	instance = new ProfileDrawer();
}


void ProfileDrawer::Disable()
{
	if (instance)
	{
		delete instance;
		instance = 0;
	}
}

void ProfileDrawer::Draw()
{
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor4f(0,0,0.5f,0.5f);
	if(!profiler.profile.empty()){
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(0.65f,0.99f,0);
		glVertex3f(0.99f,0.99f,0);
		glVertex3f(0.65f,0.99f-profiler.profile.size()*0.024f-0.01f,0);
		glVertex3f(0.99f,0.99f-profiler.profile.size()*0.024f-0.01f,0);
		glEnd();
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	std::map<std::string, CTimeProfiler::TimeRecord>::iterator pi;

	int y = 0;
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi, ++y)
		font->glFormatAt(0.655f, 0.960f - y * 0.024f, 1.0f, "%20s %6.2fs %5.2f%%", pi->first.c_str(), ((float)pi->second.total) / 1000.f, pi->second.percent * 100);

	glTranslatef(0.655f,0.965f,0);
	glScalef(0.015f,0.02f,0.02f);
	glColor4f(1,1,1,1);

	glDisable(GL_TEXTURE_2D);
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi){
		glColor3f(pi->second.color.x,pi->second.color.y,pi->second.color.z);
		glBegin(GL_QUADS);
		glVertex3f(0,0,0);
		glVertex3f(1,0,0);
		glVertex3f(1,1,0);
		glVertex3f(0,1,0);
		glEnd();
		if(!pi->second.showGraph){
			glColor3f(1,0,0);
			glBegin(GL_LINES);
			glVertex3f(0,0,0);
			glVertex3f(1,1,0);
			glVertex3f(1,0,0);
			glVertex3f(0,1,0);
			glEnd();
		}
		glTranslatef(0,-1.2f,0);
	}
	glPopMatrix();

	for(pi=profiler.profile.begin();pi!=profiler.profile.end();++pi){
		if(!pi->second.showGraph)
			continue;
		CVertexArray* va=GetVertexArray();
		va->Initialize();
		for(int a=0;a<128;++a){
			float p=((float)pi->second.frames[a])/1000.f*30;
			va->AddVertexT(float3(0.6f+a*0.003f,0.02f+p*0.4f,0),0,0);
		}
		glColor3f(pi->second.color.x,pi->second.color.y,pi->second.color.z);
		va->DrawArrayT(GL_LINE_STRIP);
	}
	glEnable(GL_TEXTURE_2D);
}

bool ProfileDrawer::MousePress(int x, int y, int button)
{
	float mx=MouseX(x);
	float my=MouseY(y);

	if(mx<0.65f || mx>0.99f || my<0.99f-profiler.profile.size()*0.024f-0.01f || my>0.99f)
		return false;

	int num=(int) ((0.99f-my)/0.024f);

	int a=0;
	std::map<std::string, CTimeProfiler::TimeRecord>::iterator pi;
	for(pi=profiler.profile.begin();pi!=profiler.profile.end() && a!=num;++pi,a++){
	}
	if(pi!=profiler.profile.end())
		pi->second.showGraph=!pi->second.showGraph;
	return false;
}

bool ProfileDrawer::IsAbove(int x, int y)
{
	const float mx=MouseX(x);
	const float my=MouseY(y);

	if(mx<0.65f || mx>0.99f || my<0.99f - profiler.profile.size()*0.024f-0.01f || my>0.99f)
		return false;

	return true;
}

ProfileDrawer::ProfileDrawer()
{
}

ProfileDrawer::~ProfileDrawer()
{
}




