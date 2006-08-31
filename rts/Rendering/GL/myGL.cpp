#include "StdAfx.h"
#include "myGL.h"
#include <GL/glu.h>
#include "Rendering/glFont.h"
#include <ostream>
#include <fstream>
#include "VertexArray.h"
#include "VertexArrayRange.h"
#include "FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Rendering/Textures/Bitmap.h"
#include "Platform/errorhandler.h"
#include "Game/UI/InfoConsole.h"
#include <SDL_video.h>
#include "mmgr.h"

#include "IFramebuffer.h"

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

	std::ofstream ofs("ext.txt");

	if (!ofs.bad() && ofs.is_open())
		ofs.write(s.c_str(), s.length());
}

void UnloadExtensions()
{
	delete vertexArray1;
	delete vertexArray2;

}

void LoadStartPicture()
{
	vector<string> bmps=CFileHandler::FindFiles("bitmaps/loadpictures/", "*.bmp");
	vector<string> jpgs=CFileHandler::FindFiles("bitmaps/loadpictures/", "*.jpg");

	int num=bmps.size()+jpgs.size();
	if(num==0)
		return;
	int selected = gu->usRandInt() % num;

	string name;
	if(selected<bmps.size())
		name=bmps[selected];
	else
		name=jpgs[selected-bmps.size()];

	CBitmap bm;
	if (!bm.Load(name))
		throw content_error("Could not load startpicture from file " + name);

	/* HACK Really big load pictures made a GLU choke. */
	if (bm.xsize > gu->screenx || bm.ysize > gu->screeny)
	{
		bm = bm.CreateRescaled(gu->screenx, gu->screeny);
	}

	startupTexture=bm.CreateTexture(false);
}

void UnloadStartPicture()
{
	if(startupTexture)
		glDeleteTextures(1,&startupTexture);
	startupTexture=0;
}

void PrintLoadMsg(const char* text, bool swapbuffers)
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
	glColor3f(1,1,1);

	if(startupTexture){
		glBindTexture(GL_TEXTURE_2D,startupTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1);glVertex2f(0,0);
			glTexCoord2f(0,0);glVertex2f(0,1);
			glTexCoord2f(1,0);glVertex2f(1,1);
			glTexCoord2f(1,1);glVertex2f(1,0);
		glEnd();
	}
	font->glPrintCentered (0.5f,0.48f, 2.0f, text);
	font->glPrintCentered(0.5f,0.06f,1.0,"TA Spring %s",VERSION_STRING);
	font->glPrintCentered(0.5f,0.02f,0.6,"This program is distributed under the GNU General Public License, see license.html for more info");
	if (swapbuffers)
		SDL_GL_SwapBuffers();
}

/**
 * True if the program in DATADIR/shaders/filename is 
 * loadable and can run inside our graphics server.
 *
 * @param target glProgramStringARB target: GL_FRAGMENT_PROGRAM_ARB GL_VERTEX_PROGRAM_ARB
 * @param filename Name of the file under shaders with the program in it.
 */
bool ProgramStringIsNative(GLenum target, const char* filename)
{
	unsigned int tempProg;
	GLint errorPos, isNative;

	if(!GLEW_ARB_vertex_program)
		return false;
	if(target==GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program)
		return false;

	CFileHandler VPFile(std::string("shaders/")+filename);
	if (!VPFile.FileExists ())
	{
		(*info) << "Warning: ProgramStringIsNative couldn't find " << filename << ".\n";
		return false;
	}
	char *VPbuf = new char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &tempProg );
	glBindProgramARB( target,tempProg);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, VPFile.FileSize(), VPbuf);

	if ( GL_INVALID_OPERATION == glGetError() )
	{
		return false;
	}
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	glDeleteProgramsARB( 1, &tempProg);

	delete[] VPbuf;
	if ((errorPos == -1) && (isNative == 1))
		return true;
	else
		return false;
}

/**
 * Presumes the last GL operation was to load a vertex or 
 * fragment program. 
 *
 * If it was invalid, display an error
 * message about what and where the problem in the program
 * is, and exit.
 *
 * @param program_type Only substituted in the message.
 * @param filename Only substituted in the message.
 */
static void CheckParseErrors(const char * program_type, const char * filename)
{
	if ( glGetError() != GL_INVALID_OPERATION )
		return;

	// Find the error position
	GLint errPos;
	glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB,&errPos );
	// Print implementation-dependent program
	// errors and warnings string.
	const GLubyte *errString=glGetString( GL_PROGRAM_ERROR_STRING_ARB);
	char c[512];
	SNPRINTF(c,512,"Error at position %d when loading %s program file %s",
		errPos,program_type,filename);
	throw content_error(c);
}

static unsigned int LoadProgram(GLenum target, const char* filename, const char * program_type)
{
	unsigned int ret;

	CFileHandler VPFile(std::string("shaders/")+filename);
	if (!VPFile.FileExists ())
	{
		char c[512];
		SNPRINTF(c,512,"Cannot find %s program file '%s'", program_type, filename);
		throw content_error(c);
	}
	char *VPbuf = new char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &ret );
	glBindProgramARB( target,ret);

	glProgramStringARB( target,GL_PROGRAM_FORMAT_ASCII_ARB,VPFile.FileSize(), VPbuf );

	CheckParseErrors(program_type, filename);
	delete[] VPbuf;
	return ret;
}

unsigned int LoadVertexProgram(const char* filename)
{
	return LoadProgram(GL_VERTEX_PROGRAM_ARB, filename, "vertex");
}

unsigned int LoadFragmentProgram(const char* filename)
{
	return LoadProgram(GL_FRAGMENT_PROGRAM_ARB, filename, "fragment");
}

IFramebuffer::~IFramebuffer() {
}
