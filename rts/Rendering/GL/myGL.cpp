#include "StdAfx.h"
#include <ostream>
#include <fstream>
#include <SDL.h>
#include "mmgr.h"

#include "myGL.h"
#include "Rendering/glFont.h"
#include "VertexArray.h"
#include "VertexArrayRange.h"
#include "FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Rendering/Textures/Bitmap.h"
#include "Platform/errorhandler.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "FPUCheck.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Exceptions.h"

#include "FBO.h"

using namespace std;


static CVertexArray* vertexArray1 = NULL;
static CVertexArray* vertexArray2 = NULL;
static CVertexArray* currentVertexArray = NULL;

static GLuint startupTexture = 0;

#ifdef USE_GML
static CVertexArray vertexArrays1[GML_MAX_NUM_THREADS];
static CVertexArray vertexArrays2[GML_MAX_NUM_THREADS];
static CVertexArray* currentVertexArrays[GML_MAX_NUM_THREADS];
#endif
//BOOL gmlVertexArrayEnable=0;
/******************************************************************************/
/******************************************************************************/

CVertexArray* GetVertexArray()
{
#ifdef USE_GML // each thread gets its own array to avoid conflicts
	int thread=gmlThreadNumber;
	if(currentVertexArrays[thread]==&vertexArrays1[thread]){
		currentVertexArrays[thread]=&vertexArrays2[thread];
	} else {
		currentVertexArrays[thread]=&vertexArrays1[thread];
	}
	return currentVertexArrays[thread];
#else
	if(currentVertexArray==vertexArray1){
		currentVertexArray=vertexArray2;
	} else {
		currentVertexArray=vertexArray1;
	}
	return currentVertexArray;
#endif
}


/******************************************************************************/

void LoadExtensions()
{
	glewInit();

	// log some useful version info
	logOutput.Print("SDL:  %d.%d.%d\n",
		SDL_Linked_Version()->major,
		SDL_Linked_Version()->minor,
		SDL_Linked_Version()->patch);
	logOutput.Print("GL:   %s\n", glGetString(GL_VERSION));
	logOutput.Print("GL:   %s\n", glGetString(GL_VENDOR));
	logOutput.Print("GL:   %s\n", glGetString(GL_RENDERER));
	logOutput.Print("GLEW: %s\n", glewGetString(GLEW_VERSION));

	/** Get available fullscreen/hardware modes **/
/*
	SDL_Rect **modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL|SDL_RESIZABLE);

	if (modes == (SDL_Rect **)0) {
		logOutput.Print("SDL_ListModes: No modes available!\n");
	}else if (modes == (SDL_Rect **)-1) {
		logOutput.Print("SDL_ListModes: Resolution is restricted.\n");
	}else{
		char buffer[512];
		unsigned char n = 0;
		for(int i=0;modes[i];++i) {
			n += SNPRINTF(&buffer[n], 512-n, "%dx%d, ", modes[i]->w, modes[i]->h);
		}
		logOutput.Print("SDL_ListModes: %s\n",buffer);
	}
*/

	std::string missingExts = "";
	if(!GL_ARB_fragment_program_shadow) {
		// used by the ground shadow shaders
		// if this is not found, the installed graphic drivers most likely
		// have no OpenGL support at all, eg. VESA
		missingExts += " GL_ARB_fragment_program_shadow";
	}
	if(!GLEW_ARB_multitexture) {
		missingExts += " GLEW_ARB_multitexture";
	}
	if(!GLEW_ARB_texture_env_combine) {
		missingExts += " GLEW_ARB_texture_env_combine";
	}
	if(!GLEW_ARB_texture_compression) {
		missingExts += " GLEW_ARB_texture_compression";
	}

	if(!missingExts.empty()) {
		static const unsigned int errorMsg_maxSize = 2048;
		char errorMsg[errorMsg_maxSize];
		SNPRINTF(errorMsg, errorMsg_maxSize,
				"Needed OpenGL extension(s) not found:\n"
				"%s\n"
				"Update your graphic-card driver!\n"
				"graphic card:   %s\n"
				"OpenGL version: %s\n",
				missingExts.c_str(), (const char*)glGetString(GL_RENDERER),
				(const char*)glGetString(GL_RENDERER));
		handleerror(0, errorMsg, "Update graphic drivers", 0);
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


/******************************************************************************/

void glBuildMipmaps(const GLenum target,GLint internalFormat,const GLsizei width,const GLsizei height,const GLenum format,const GLenum type,const void *data)
{
	if (gu->compressTextures) {
		switch ( internalFormat ) {
			case 4:
			case GL_RGBA8 :
			case GL_RGBA :  internalFormat = GL_COMPRESSED_RGBA_ARB; break;

			case 3:
			case GL_RGB8 :
			case GL_RGB :   internalFormat = GL_COMPRESSED_RGB_ARB; break;

			case GL_LUMINANCE: internalFormat = GL_COMPRESSED_LUMINANCE_ARB; break;
		}
	}

	// create mipmapped texture

	if (glGenerateMipmapEXT_NONGML) { // broken on ATIs and NVs (wait for their OpenGL3.0 drivers :/)
		// newest method
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
		if (gu->atiHacks) {
			glEnable(target);
			glGenerateMipmapEXT(target);
			glDisable(target);
		}else{
			glGenerateMipmapEXT(target);
		}
	}else if (GLEW_VERSION_1_4) {
		// This required GL-1.4
		// instead of using glu, we rely on glTexImage2D to create the Mipmaps.
		glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	} else
		gluBuild2DMipmaps(target, internalFormat, width, height, format, type, data);
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
	if (startupTexture)
		return;
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

void ClearScreen()
{
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
	glLoadIdentity();            // Reset The Projection Matrix
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);  // Select The Modelview Matrix

	glLoadIdentity();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
}

void PrintLoadMsg(const char* text, bool swapbuffers)
{
	static char prevText[100];

	// Stuff that needs to be done regularly while loading.
	// Totally unrelated to the task the name of this function implies.

	// Check to prevent infolog spam by CPreGame which uses this function
	// to render the screen background each frame.
	if (strcmp(prevText, text)) {
		logOutput.Print("%s",text);
		strncpy(prevText, text, sizeof(prevText));
		prevText[sizeof(prevText) - 1] = 0;
	}

	good_fpu_control_registers(text);

	// Draw loading screen & print load msg.
	ClearScreen();
	if (startupTexture) {
		glBindTexture(GL_TEXTURE_2D,startupTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1);glVertex2f(0,0);
			glTexCoord2f(0,0);glVertex2f(0,1);
			glTexCoord2f(1,0);glVertex2f(1,1);
			glTexCoord2f(1,1);glVertex2f(1,0);
		glEnd();
	}

	font->SetOutlineColor(0.0f,0.0f,0.0f,0.65f);
	font->SetTextColor(1.0f,1.0f,1.0f,1.0f);

	font->glPrint(0.5f,0.5f,   gu->viewSizeY / 15.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM, text);
#ifdef USE_GML
	font->glFormat(0.5f,0.06f, gu->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM, "Spring %s MT (%d threads)", SpringVersion::GetFull().c_str(), gmlThreadCount);
#else
	font->glFormat(0.5f,0.06f, gu->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM, "Spring %s", SpringVersion::GetFull().c_str());
#endif
	font->glFormat(0.5f,0.02f, gu->viewSizeY / 50.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM, "This program is distributed under the GNU General Public License, see license.html for more info");

	if (swapbuffers) {
		SDL_GL_SwapBuffers();
	}
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
	GLuint tempProg;
	GLint errorPos, isNative;

	if(!GLEW_ARB_vertex_program)
		return false;
	if(target==GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program)
		return false;

	CFileHandler VPFile(std::string("shaders/")+filename);
	if (!VPFile.FileExists ())
	{
		LogObject() << "Warning: ProgramStringIsNative couldn't find " << filename << ".\n";
		return false;
	}
	char *VPbuf = new char[VPFile.FileSize()];
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
 * @param program The program text (used to enhance the message)
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
	static const unsigned int errorMsg_maxSize = 2048;
	char errorMsg[errorMsg_maxSize];
	SNPRINTF(errorMsg, errorMsg_maxSize,
			"Error at position %d near \"%.30s\" when loading %s program file %s:\n%s",
			errPos, program+errPos, program_type, filename, (const char*)errString);
	throw content_error(errorMsg);
}


static unsigned int LoadProgram(GLenum target, const char* filename, const char * program_type)
{
	GLuint ret = 0;
	if (!GLEW_ARB_vertex_program) {
		return 0;
	}
	if(target==GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program) {
		return 0;
	}

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
