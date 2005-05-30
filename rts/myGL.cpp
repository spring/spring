#include "StdAfx.h"
#include "myGL.h"
#include "GL/glu.h"
#include "glFont.h"
#include <ostream>
#include <fstream>
#include <math.h>
#include "VertexArray.h"
#include "VertexArrayRange.h"
#include "FileHandler.h"
//#include "mmgr.h"

using namespace std;

extern HDC hDC;
#ifndef NO_WINSTUFF
extern HWND	hWnd;
#endif

static CVertexArray* vertexArray1=0;
static CVertexArray* vertexArray2=0;
static CVertexArray* currentVertexArray=0;

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
	/*
	if (glewGetExtension("GL_ARB_texture_env_combine")==GL_FALSE)
	{
		MessageBox(0,"Needed extension GL_ARB_texture_env_combine not found","Update drivers",0);
		exit(1);	  	
	}	

	if (glewGetExtension("GL_ARB_multitexture")==GL_FALSE)
	{
		MessageBox(0,"Needed extension GL_ARB_texture_env_combine not found","Update drivers",0);
		exit(1);	  	
	}	

	if (glewGetExtension("GL_ARB_texture_compression")==GL_FALSE)
	{
		MessageBox(0,"Needed extension GL_ARB_texture_env_combine not found","Update drivers",0);
		exit(1);	  	
	}		*/
	
	vertexArray1=new CVertexArray;
	vertexArray2=new CVertexArray;

	std::string s= (char*)glGetString(GL_EXTENSIONS);
	for (unsigned int i=0; i<s.length(); i++) 
		if (s[i]==' ') s[i]='\n';

	ofstream ofs("ext.txt",ios::out);
	ofs.write(s.c_str(),s.length());
}

void UnloadExtensions()
{
	delete vertexArray1;
	delete vertexArray2;

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
	glPushMatrix();
	glTranslatef(0.5f-0.01f*strlen(text),0.48f,0.0f);
	glScalef(0.03f,0.04f,0.1f);
	glColor3f(1,1,1);
	font->glPrint("%s",text);
	glPopMatrix();
	font->glPrintAt(0.40,0.06,1.0,"TA Spring 0.41b1");
	font->glPrintAt(0.20,0.02,0.5,"This program is distributed under the GNU General Public License, see license.html for more info");
#ifndef NO_WINDOWS
	SwapBuffers(hDC);
#endif
}

bool ProgramStringIsNative(GLenum target, const char* filename)
{
	unsigned int tempProg;
	GLint errorPos, isNative;

	if(!GLEW_ARB_vertex_program)
		return false;
	if(target==GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program)
		return false;

	CFileHandler VPFile(std::string("shaders\\")+filename);
	char *VPbuf = new char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &tempProg );
	glBindProgramARB( target,tempProg);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, VPFile.FileSize(), VPbuf);

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	glDeleteProgramsARB( 1, &tempProg);

	delete VPbuf;
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
		MessageBox(0,c,(char*)errString,0);	
		exit(0);
	}
}

unsigned int LoadVertexProgram(const char* filename)
{
	unsigned int ret;

	CFileHandler VPFile(std::string("shaders\\")+filename);
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
		MessageBox(0,c,(char*)errString,0);	
		exit(0);
	}
	delete VPbuf;
	return ret;
}

unsigned int LoadFragmentProgram(const char* filename)
{
	unsigned int ret;

	CFileHandler VPFile(std::string("shaders\\")+filename);
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
		MessageBox(0,c,(char*)errString,0);	
		exit(0);
	}
	delete VPbuf;
	return ret;
}
