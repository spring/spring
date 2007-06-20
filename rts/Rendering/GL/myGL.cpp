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
#include "LogOutput.h"
#include "FPUCheck.h"
#include <SDL.h>
#include "NetProtocol.h"
#include "mmgr.h"

#include "IFramebuffer.h"

using namespace std;


static CVertexArray* vertexArray1 = 0;
static CVertexArray* vertexArray2 = 0;
static CVertexArray* currentVertexArray = 0;

static unsigned int startupTexture = 0;


/******************************************************************************/
/******************************************************************************/

CVertexArray* GetVertexArray()
{
	if(currentVertexArray==vertexArray1){
		currentVertexArray=vertexArray2;
	} else {
		currentVertexArray=vertexArray1;
	}
	return currentVertexArray;
}


/******************************************************************************/

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

	vertexArray1=SAFE_NEW CVertexArray;
	vertexArray2=SAFE_NEW CVertexArray;

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


/******************************************************************************/

static void AppendStringVec(vector<string>& dst, const vector<string>& src)
{
	for (int i = 0; i < (int)src.size(); i++) {
		dst.push_back(src[i]);
	}
}


static string SelectPicture(const std::string& dir, const std::string& prefix)
{
	vector<string> pics;

	AppendStringVec(pics, CFileHandler::FindFiles(dir, prefix + "*.bmp"));
	AppendStringVec(pics, CFileHandler::FindFiles(dir, prefix + "*.jpg"));

	// add 'allside_' pictures if we have a prefix
	if (!prefix.empty()) {
		AppendStringVec(pics, CFileHandler::FindFiles(dir, "allside_*.bmp"));
		AppendStringVec(pics, CFileHandler::FindFiles(dir, "allside_*.jpg"));
	}

	if (pics.empty()) {
		return "";
	}

	return pics[gu->usRandInt() % pics.size()];
}


void LoadStartPicture(const std::string& sidePref)
{
	const string picDir = "bitmaps/loadpictures/";

	string name = "";
	if (!sidePref.empty()) {
		name = SelectPicture(picDir, sidePref + "_");
	}
	if (name.empty()) {
		name = SelectPicture(picDir, "");
	}
	if (name.empty()) {
		return; // no valid pictures
	}

	CBitmap bm;
	if (!bm.Load(name)) {
		throw content_error("Could not load startpicture from file " + name);
	}

	/* HACK Really big load pictures made a GLU choke. */
	if ((bm.xsize > gu->viewSizeX) || (bm.ysize > gu->viewSizeY)) {
		bm = bm.CreateRescaled(gu->viewSizeX, gu->viewSizeY);
	}

	startupTexture = bm.CreateTexture(false);
}


void UnloadStartPicture()
{
	if (startupTexture) {
		glDeleteTextures(1, &startupTexture);
	}
	startupTexture = 0;
}


/******************************************************************************/

void PrintLoadMsg(const char* text, bool swapbuffers)
{
	static char prevText[100];
	static unsigned startTicks;

	PUSH_CODE_MODE;

	// Stuff that needs to be done regularly while loading.
	// Totally unrelated to the task the name of this function implies.
	ENTER_MIXED;

	// Check to prevent infolog spam by CPreGame which uses this function
	// to render the screen background each frame.
	if (strcmp(prevText, text)) {
		unsigned ticks = SDL_GetTicks();
//		if (prevText[0])
//			logOutput.Print("Loading step `%s' took %g seconds", prevText, (ticks - startTicks) / 1000.0f);
		logOutput.Print("%s",text);
		strncpy(prevText, text, sizeof(prevText));
		prevText[sizeof(prevText) - 1] = 0;
		startTicks = ticks;
	}

	good_fpu_control_registers(text);

	net->Update();	//prevent timing out during load

	// Draw loading screen & print load msg.
	ENTER_UNSYNCED;

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
	font->glPrintCentered(0.5f,0.06f,1.0f,"Spring %s", VERSION_STRING);
	font->glPrintCentered(0.5f,0.02f,0.6f,"This program is distributed under the GNU General Public License, see license.html for more info");
	if (swapbuffers)
		SDL_GL_SwapBuffers();
	POP_CODE_MODE;
}


/******************************************************************************/
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
		logOutput << "Warning: ProgramStringIsNative couldn't find " << filename << ".\n";
		return false;
	}
	char *VPbuf = SAFE_NEW char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	// clear any current GL errors so that the following check is valid
	glClearErrors();

	glGenProgramsARB( 1, &tempProg );
	glBindProgramARB( target,tempProg);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, VPFile.FileSize(), VPbuf);

	if ( GL_INVALID_OPERATION == glGetError() )
	{
		return false;
	}
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	glSafeDeleteProgram( tempProg);

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
static void CheckParseErrors(const char * program_type, const char * filename, const char* program)
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
	SNPRINTF(c,512,"Error at position %d near \"%.30s\" when loading %s program file %s",
		errPos, program+errPos, program_type, filename);
	throw content_error(c);
}


static unsigned int LoadProgram(GLenum target, const char* filename, const char * program_type)
{
	unsigned int ret = 0;
	if (!GLEW_ARB_vertex_program) {
		return 0;
	}

	CFileHandler VPFile(std::string("shaders/")+filename);
	if (!VPFile.FileExists ())
	{
		char c[512];
		SNPRINTF(c,512,"Cannot find %s program file '%s'", program_type, filename);
		throw content_error(c);
	}
	char *VPbuf = SAFE_NEW char[VPFile.FileSize()];
	VPFile.Read(VPbuf, VPFile.FileSize());

	glGenProgramsARB( 1, &ret );
	glBindProgramARB( target,ret);

	glProgramStringARB( target,GL_PROGRAM_FORMAT_ASCII_ARB,VPFile.FileSize(), VPbuf );

	CheckParseErrors(program_type, filename, VPbuf);
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

void glSafeDeleteProgram(GLuint program)
{
	if (!GLEW_ARB_vertex_program || (program == 0)) {
		return;
	}
	glDeleteProgramsARB(1, &program);
}


/******************************************************************************/

void glClearErrors()
{
	int safety = 0;
	while ((glGetError() != GL_NO_ERROR) && (safety < 1000)) {
		safety++;
	}
}


/******************************************************************************/

IFramebuffer::~IFramebuffer() {
}


/******************************************************************************/
/******************************************************************************/
