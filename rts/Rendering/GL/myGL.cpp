#include "System/StdAfx.h"
#include "myGL.h"
#include <GL/glu.h>
#include "Rendering/glFont.h"
#include <ostream>
#include <fstream>
#include <math.h>
#include "VertexArray.h"
#include "VertexArrayRange.h"
#include "System/FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "System/Bitmap.h"
#include "System/Platform/errorhandler.h"
#include <boost/filesystem/path.hpp>
#include "SDL_video.h"
//#include "System/mmgr.h"

using namespace std;

static CVertexArray* vertexArray1=0;
static CVertexArray* vertexArray2=0;
static CVertexArray* currentVertexArray=0;

static unsigned int startupTexture=0;

CVertexArray* GetVertexArray()
{
	if(currentVertexArray==vertexArray1){
		currentVertexArray=vertexArray2;
	} else {
		currentVertexArray=vertexArray1;
	}
	return currentVertexArray;
}

void LoadExtensions()
{
	glewInit();

	if(!GLEW_ARB_multitexture || !GLEW_ARB_texture_env_combine){
		handleerror(0,"Needed extension GL_ARB_texture_env_combine not found","Update drivers",0);
		exit(0);
	}

	if(!GLEW_ARB_texture_compression){
		handleerror(0,"Needed extension GL_ARB_texture_compression not found","Update drivers",0);
		exit(0);
	}
	
	vertexArray1=new CVertexArray;
	vertexArray2=new CVertexArray;

	std::string s= (char*)glGetString(GL_EXTENSIONS);
	for (unsigned int i=0; i<s.length(); i++) 
		if (s[i]==' ') s[i]='\n';

	boost::filesystem::path fn("ext.txt");
	ofstream ofs(fn.native_file_string().c_str(),ios::out);
	ofs.write(s.c_str(),s.length());
}

void UnloadExtensions()
{
	delete vertexArray1;
	delete vertexArray2;

}

void LoadStartPicture()
{
	vector<string> bmps=CFileHandler::FindFiles("bitmaps/loadpictures/*.bmp");
	vector<string> jpgs=CFileHandler::FindFiles("bitmaps/loadpictures/*.jpg");

	int num=bmps.size()+jpgs.size();
	if(num==0)
		return;
	int selected=(int)(gu->usRandFloat()*(num-0.01));

	string name;
	if(selected<bmps.size())
		name=bmps[selected];
	else
		name=jpgs[selected-bmps.size()];

	CBitmap bm(name);
	startupTexture=bm.CreateTexture(true);
}

void UnloadStartPicture()
{
	if(startupTexture)
		glDeleteTextures(1,&startupTexture);
	startupTexture=0;
}

void PrintLoadMsg(const char* text)
{
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix

	glLoadIdentity();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	if(startupTexture){
		glBindTexture(GL_TEXTURE_2D,startupTexture);
		glColor3f(1,1,1);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1);glVertex2f(0,0);
			glTexCoord2f(0,0);glVertex2f(0,1);
			glTexCoord2f(1,0);glVertex2f(1,1);
			glTexCoord2f(1,1);glVertex2f(1,0);
		glEnd();
	}
	glPushMatrix();
	glTranslatef(0.5f-0.01f*strlen(text),0.48f,0.0f);
	glScalef(0.03f,0.04f,0.1f);
	glColor3f(1,1,1);
	font->glPrint("%s",text);
	glPopMatrix();
	font->glPrintAt(0.40,0.06,1.0,"TA Spring linux %s",VERSION_STRING);
	font->glPrintAt(0.20,0.02,0.5,"This program is distributed under the GNU General Public License, see license.html for more info");
	SDL_GL_SwapBuffers();
}

bool ProgramStringIsNative(GLenum target, const char* filename)
{
	unsigned int tempProg;
	GLint errorPos, isNative;

	if(!GLEW_ARB_vertex_program)
		return false;
	if(target==GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program)
		return false;

	CFileHandler VPFile(std::string("shaders/")+filename);
	char *VPbuf = new char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &tempProg );
	glBindProgramARB( target,tempProg);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, VPFile.FileSize(), VPbuf);

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	glDeleteProgramsARB( 1, &tempProg);

	delete[] VPbuf;
	if ((errorPos == -1) && (isNative == 1))
		return true;
	else
		return false;
}

void CheckParseErrors()
{
	if ( GL_INVALID_OPERATION == glGetError() )
	{
		// Find the error position
		GLint errPos;
		glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB,&errPos );
		// Print implementation-dependent program
		// errors and warnings string.
		const GLubyte *errString=glGetString( GL_PROGRAM_ERROR_STRING_ARB);
		char c[512];
		sprintf(c,"Error at position %d",errPos);
		handleerror(0,c,(char*)errString,0);	
		exit(0);
	}
}

unsigned int LoadVertexProgram(const char* filename)
{
	unsigned int ret;

	CFileHandler VPFile(std::string("shaders/")+filename);
	if (!VPFile.FileExists ())
		return 0;
	char *VPbuf = new char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &ret );
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB,ret);

	glProgramStringARB( GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,VPFile.FileSize(), VPbuf );

	if ( GL_INVALID_OPERATION == glGetError() )
	{
		// Find the error position
		GLint errPos;
		glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB,&errPos );
		// Print implementation-dependent program
		// errors and warnings string.
		const GLubyte *errString=glGetString( GL_PROGRAM_ERROR_STRING_ARB);
		char c[512];
		sprintf(c,"Error at position %d when loading vertex program file %s",errPos,filename);
		handleerror(0,c,(char*)errString,0);	
		exit(0);
	}
	delete[] VPbuf;
	return ret;
}

unsigned int LoadFragmentProgram(const char* filename)
{
	unsigned int ret;

	CFileHandler VPFile(std::string("shaders/")+filename);
	if (!VPFile.FileExists ())
		return 0;
	char *VPbuf = new char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &ret );
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB,ret);

	glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,VPFile.FileSize(), VPbuf );

	if ( GL_INVALID_OPERATION == glGetError() )
	{
		// Find the error position
		GLint errPos;
		glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB,&errPos );
		// Print implementation-dependent program
		// errors and warnings string.
		const GLubyte *errString=glGetString( GL_PROGRAM_ERROR_STRING_ARB);
		char c[512];
		sprintf(c,"Error at position %d when loading fragment program file %s",errPos,filename);
		handleerror(0,c,(char*)errString,0);	
		exit(0);
	}
	delete[] VPbuf;
	return ret;
}
