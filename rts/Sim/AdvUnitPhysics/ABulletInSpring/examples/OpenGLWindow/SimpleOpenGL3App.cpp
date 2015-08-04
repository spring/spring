#ifndef NO_OPENGL3

#include "SimpleOpenGL3App.h"
#include "ShapeData.h"
#ifdef __APPLE__
#include "MacOpenGLWindow.h"
#else



#ifdef _WIN32
#include "Win32OpenGLWindow.h"
#else
//let's cross the fingers it is Linux/X11
#include "X11OpenGLWindow.h"
#endif //_WIN32
#endif//__APPLE__
#include <stdio.h>

#include "GLPrimitiveRenderer.h"
#include "GLInstancingRenderer.h"

#include "Bullet3Common/b3Vector3.h"
#include "Bullet3Common/b3Logging.h"

#include "fontstash.h"
#include "TwFonts.h"
#include "opengl_fontstashcallbacks.h"
#include <assert.h>
#include "GLRenderToTexture.h"
#include "Bullet3Common/b3Quaternion.h"

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif // _WIN32

struct SimpleInternalData
{
	GLuint m_fontTextureId;
	GLuint m_largeFontTextureId;
	struct sth_stash* m_fontStash;
	OpenGL2RenderCallbacks*		m_renderCallbacks;
	int m_droidRegular;
	const char* m_frameDumpPngFileName;
	FILE* m_ffmpegFile;
	GLRenderToTexture*  m_renderTexture;
	void* m_userPointer;
	int m_upAxis;//y=1 or z=2 is supported

};

static SimpleOpenGL3App* gApp=0;

static void SimpleResizeCallback( float widthf, float heightf)
{
	int width = (int)widthf;
	int height = (int)heightf;
	gApp->m_instancingRenderer->resize(width,height);
	gApp->m_primRenderer->setScreenSize(width,height);

}

static void SimpleKeyboardCallback(int key, int state)
{
    if (key==B3G_ESCAPE && gApp && gApp->m_window)
    {
        gApp->m_window->setRequestExit();
    } else
    {
        //gApp->defaultKeyboardCallback(key,state);
    }
}

void SimpleMouseButtonCallback( int button, int state, float x, float y)
{
	gApp->defaultMouseButtonCallback(button,state,x,y);
}
void SimpleMouseMoveCallback(  float x, float y)
{
	gApp->defaultMouseMoveCallback(x,y);
}
	
void SimpleWheelCallback( float deltax, float deltay)
{
	gApp->defaultWheelCallback(deltax,deltay);
}




static GLuint BindFont(const CTexFont *_Font)
{
    GLuint TexID = 0;
    glGenTextures(1, &TexID);
    glBindTexture(GL_TEXTURE_2D, TexID);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _Font->m_TexWidth, _Font->m_TexHeight, 0, GL_RED, GL_UNSIGNED_BYTE, _Font->m_TexBytes);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return TexID;
}

extern unsigned char OpenSansData[];

SimpleOpenGL3App::SimpleOpenGL3App(	const char* title, int width,int height)
{
	gApp = this;
	m_data = new SimpleInternalData;
	m_data->m_frameDumpPngFileName = 0;
	m_data->m_renderTexture = 0;
	m_data->m_ffmpegFile = 0;
	m_data->m_userPointer = 0;
	m_data->m_upAxis = 1;

	m_window = new b3gDefaultOpenGLWindow();
	b3gWindowConstructionInfo ci;
	ci.m_title = title;
	ci.m_width = width;
	ci.m_height = height;
	m_window->createWindow(ci);

	m_window->setWindowTitle(title);

	b3Assert(glGetError() ==GL_NO_ERROR);

	glClearColor(0.9,0.9,1,1);
	m_window->startRendering();
	b3Assert(glGetError() ==GL_NO_ERROR);

#ifndef __APPLE__
#ifndef _WIN32
//some Linux implementations need the 'glewExperimental' to be true
    glewExperimental = GL_TRUE;
#endif


    if (glewInit() != GLEW_OK)
        exit(1); // or handle the error in a nicer way
    if (!GLEW_VERSION_2_1)  // check that the machine supports the 2.1 API.
        exit(1); // or handle the error in a nicer way

#endif
    glGetError();//don't remove this call, it is needed for Ubuntu

    b3Assert(glGetError() ==GL_NO_ERROR);

	m_primRenderer = new GLPrimitiveRenderer(width,height);
	m_parameterInterface = 0;

    b3Assert(glGetError() ==GL_NO_ERROR);

	m_instancingRenderer = new GLInstancingRenderer(128*1024,64*1024*1024);
	m_renderer = m_instancingRenderer ;
	m_instancingRenderer->init();
	m_instancingRenderer->resize(width,height);

	b3Assert(glGetError() ==GL_NO_ERROR);

	m_instancingRenderer->InitShaders();

	m_window->setMouseMoveCallback(SimpleMouseMoveCallback);
	m_window->setMouseButtonCallback(SimpleMouseButtonCallback);
    m_window->setKeyboardCallback(SimpleKeyboardCallback);
    m_window->setWheelCallback(SimpleWheelCallback);
	m_window->setResizeCallback(SimpleResizeCallback);

	TwGenerateDefaultFonts();
	m_data->m_fontTextureId = BindFont(g_DefaultNormalFont);
	m_data->m_largeFontTextureId = BindFont(g_DefaultLargeFont);
	


	{



	m_data->m_renderCallbacks = new OpenGL2RenderCallbacks(m_primRenderer);
	m_data->m_fontStash = sth_create(512,512,m_data->m_renderCallbacks);//256,256);//,1024);//512,512);
    b3Assert(glGetError() ==GL_NO_ERROR);

	if (!m_data->m_fontStash)
	{
		b3Warning("Could not create stash");
		//fprintf(stderr, "Could not create stash.\n");
	}


	unsigned char* data2 = OpenSansData;
	unsigned char* data = (unsigned char*) data2;
	if (!(m_data->m_droidRegular = sth_add_font_from_memory(m_data->m_fontStash, data)))
	{
		b3Warning("error!\n");
	}
    b3Assert(glGetError() ==GL_NO_ERROR);
	}
}


struct sth_stash* SimpleOpenGL3App::getFontStash()
{
	return m_data->m_fontStash;
}

void SimpleOpenGL3App::drawText3D( const char* txt, float worldPosX, float worldPosY, float worldPosZ, float size1)
{

	float viewMat[16];
	float projMat[16];
	CommonCameraInterface* cam = m_instancingRenderer->getActiveCamera();

	cam->getCameraViewMatrix(viewMat);
	cam->getCameraProjectionMatrix(projMat);

	
	float camPos[4];
	cam->getCameraPosition(camPos);
	//b3Vector3 cp= b3MakeVector3(camPos[0],camPos[2],camPos[1]);
	//b3Vector3 p = b3MakeVector3(worldPosX,worldPosY,worldPosZ);
	//float dist = (cp-p).length();
	//float dv = 0;//dist/1000.f;
    //
    //printf("str = %s\n",unicodeText);

    float dx=0;

    //int measureOnly=0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int viewport[4]={0,0,m_instancingRenderer->getScreenWidth(),m_instancingRenderer->getScreenHeight()};

	float posX = 450.f;
	float posY = 100.f;
	float winx,winy, winz;

	if (!projectWorldCoordToScreen(worldPosX, worldPosY, worldPosZ,viewMat,projMat,viewport,&winx, &winy, &winz))
	{
		return;
	}
	posX = winx;
	posY = m_instancingRenderer->getScreenHeight()/2+(m_instancingRenderer->getScreenHeight()/2)-winy;

	
	if (0)//m_useTrueTypeFont)
	{
		bool measureOnly = false;

		float fontSize= 32;//64;//512;//128;
		sth_draw_text(m_data->m_fontStash,
                    m_data->m_droidRegular,fontSize,posX,posY,
					txt,&dx, this->m_instancingRenderer->getScreenWidth(),this->m_instancingRenderer->getScreenHeight(),measureOnly,m_window->getRetinaScale());
		sth_end_draw(m_data->m_fontStash);
		sth_flush_draw(m_data->m_fontStash);
	} else
	{
		//float width = 0.f;
		int pos=0;
		//float color[]={0.2f,0.2,0.2f,1.f};
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,m_data->m_largeFontTextureId);

		//float width = r.x;
		//float extraSpacing = 0.;

		float startX = posX;
		float startY = posY-g_DefaultLargeFont->m_CharHeight*size1;
		

		while (txt[pos])
		{
			int c = txt[pos];
			//r.h = g_DefaultNormalFont->m_CharHeight;
			//r.w = g_DefaultNormalFont->m_CharWidth[c]+extraSpacing;
			float endX = startX+g_DefaultLargeFont->m_CharWidth[c]*size1;
			float endY = posY;


			float currentColor[]={1.f,0.2,0.2f,1.f};

		//	m_primRenderer->drawTexturedRect(startX, startY, endX, endY, currentColor,g_DefaultLargeFont->m_CharU0[c],g_DefaultLargeFont->m_CharV0[c],g_DefaultLargeFont->m_CharU1[c],g_DefaultLargeFont->m_CharV1[c]);
			float u0 = g_DefaultLargeFont->m_CharU0[c];
			float u1 = g_DefaultLargeFont->m_CharU1[c];
			float v0 = g_DefaultLargeFont->m_CharV0[c];
			float v1 = g_DefaultLargeFont->m_CharV1[c];
			float color[4] = {currentColor[0],currentColor[1],currentColor[2],currentColor[3]};
			float x0 = startX;
			float x1 = endX;
			float y0 = startY;
			float y1 = endY;
			int screenWidth = m_instancingRenderer->getScreenWidth();
			int screenHeight = m_instancingRenderer->getScreenHeight();

			float z = 2.f*winz-1.f;//*(far
			 float identity[16]={1,0,0,0,
						0,1,0,0,
						0,0,1,0,
						0,0,0,1};
				   PrimVertex vertexData[4] = {
					{ PrimVec4(-1.f+2.f*x0/float(screenWidth), 1.f-2.f*y0/float(screenHeight), z, 1.f ), PrimVec4( color[0], color[1], color[2], color[3] ) ,PrimVec2(u0,v0)},
					{ PrimVec4(-1.f+2.f*x0/float(screenWidth),  1.f-2.f*y1/float(screenHeight), z, 1.f ), PrimVec4( color[0], color[1], color[2], color[3] ) ,PrimVec2(u0,v1)},
					{ PrimVec4( -1.f+2.f*x1/float(screenWidth),  1.f-2.f*y1/float(screenHeight), z, 1.f ), PrimVec4(color[0], color[1], color[2], color[3]) ,PrimVec2(u1,v1)},
					{ PrimVec4( -1.f+2.f*x1/float(screenWidth), 1.f-2.f*y0/float(screenHeight), z, 1.f ), PrimVec4( color[0], color[1], color[2], color[3] ) ,PrimVec2(u1,v0)}
				};
    
				m_primRenderer->drawTexturedRect3D(vertexData[0],vertexData[1],vertexData[2],vertexData[3],identity,identity,false);

			//DrawTexturedRect(0,r,g_DefaultNormalFont->m_CharU0[c],g_DefaultNormalFont->m_CharV0[c],g_DefaultNormalFont->m_CharU1[c],g_DefaultNormalFont->m_CharV1[c]);
		//	DrawFilledRect(r);

			startX = endX;
			//startY = endY;

			pos++;

		}
		glBindTexture(GL_TEXTURE_2D,0);
	}

	glDisable(GL_BLEND);



}


void SimpleOpenGL3App::drawText( const char* txt, int posXi, int posYi)
{

	float posX = (float)posXi;
	float posY = (float) posYi;


    //
    //printf("str = %s\n",unicodeText);

    float dx=0;

    //int measureOnly=0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	if (1)//m_useTrueTypeFont)
	{
		bool measureOnly = false;

		float fontSize= 64;//512;//128;
		sth_draw_text(m_data->m_fontStash,
                    m_data->m_droidRegular,fontSize,posX,posY,
					txt,&dx, this->m_instancingRenderer->getScreenWidth(),
					this->m_instancingRenderer->getScreenHeight(),
					measureOnly,
					m_window->getRetinaScale());
					
		sth_end_draw(m_data->m_fontStash);
		sth_flush_draw(m_data->m_fontStash);
	} else
	{
		//float width = 0.f;
		int pos=0;
		//float color[]={0.2f,0.2,0.2f,1.f};
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,m_data->m_largeFontTextureId);

		//float width = r.x;
		//float extraSpacing = 0.;

		float startX = posX;
		float startY = posY;


		while (txt[pos])
		{
			int c = txt[pos];
			//r.h = g_DefaultNormalFont->m_CharHeight;
			//r.w = g_DefaultNormalFont->m_CharWidth[c]+extraSpacing;
			float endX = startX+g_DefaultLargeFont->m_CharWidth[c];
			float endY = startY+g_DefaultLargeFont->m_CharHeight;


			float currentColor[]={0.2f,0.2,0.2f,1.f};

			m_primRenderer->drawTexturedRect(startX, startY, endX, endY, currentColor,g_DefaultLargeFont->m_CharU0[c],g_DefaultLargeFont->m_CharV0[c],g_DefaultLargeFont->m_CharU1[c],g_DefaultLargeFont->m_CharV1[c]);

			//DrawTexturedRect(0,r,g_DefaultNormalFont->m_CharU0[c],g_DefaultNormalFont->m_CharV0[c],g_DefaultNormalFont->m_CharU1[c],g_DefaultNormalFont->m_CharV1[c]);
		//	DrawFilledRect(r);

			startX = endX;
			//startY = endY;

			pos++;

		}
		glBindTexture(GL_TEXTURE_2D,0);
	}

	glDisable(GL_BLEND);
}

struct GfxVertex
	{
		float x,y,z,w;
		float nx,ny,nz;
		float u,v;
	};

int	SimpleOpenGL3App::registerCubeShape(float halfExtentsX,float halfExtentsY, float halfExtentsZ)
{


	int strideInBytes = 9*sizeof(float);
	int numVertices = sizeof(cube_vertices)/strideInBytes;
	int numIndices = sizeof(cube_indices)/sizeof(int);

	b3AlignedObjectArray<GfxVertex> verts;
	verts.resize(numVertices);
	for (int i=0;i<numVertices;i++)
	{
		verts[i].x = halfExtentsX*cube_vertices[i*9];
		verts[i].y = halfExtentsY*cube_vertices[i*9+1];
		verts[i].z = halfExtentsZ*cube_vertices[i*9+2];
		verts[i].w = cube_vertices[i*9+3];
		verts[i].nx = cube_vertices[i*9+4];
		verts[i].ny = cube_vertices[i*9+5];
		verts[i].nz = cube_vertices[i*9+6];
		verts[i].u = cube_vertices[i*9+7];
		verts[i].v = cube_vertices[i*9+8];
	}
	
	int shapeId = m_instancingRenderer->registerShape(&verts[0].x,numVertices,cube_indices,numIndices);
	return shapeId;
}

void SimpleOpenGL3App::registerGrid(int cells_x, int cells_z, float color0[4], float color1[4])
{
	b3Vector3 cubeExtents=b3MakeVector3(0.5,0.5,0.5);
	cubeExtents[m_data->m_upAxis] = 0;
	int cubeId = registerCubeShape(cubeExtents[0],cubeExtents[1],cubeExtents[2]);

	b3Quaternion orn(0,0,0,1);
	b3Vector3 center=b3MakeVector3(0,0,0,1);
	b3Vector3 scaling=b3MakeVector3(1,1,1,1);

	for ( int i = 0; i < cells_x; i++) 
	{
		for (int j = 0; j < cells_z; j++) 
		{
			float* color =0;
			if ((i + j) % 2 == 0) 
			{
				color = (float*)color0;
			} else {
				color = (float*)color1;
			}
			if (this->m_data->m_upAxis==1)
			{
				center =b3MakeVector3((i + 0.5f) - cells_x * 0.5f, 0.f, (j + 0.5f) - cells_z * 0.5f);
			} else
			{
				center =b3MakeVector3((i + 0.5f) - cells_x * 0.5f, (j + 0.5f) - cells_z * 0.5f,0.f );
			}
			m_instancingRenderer->registerGraphicsInstance(cubeId,center,orn,color,scaling);
		}
	}
	
}


int	SimpleOpenGL3App::registerGraphicsSphereShape(float radius, bool usePointSprites, int largeSphereThreshold, int mediumSphereThreshold)
{

	int strideInBytes = 9*sizeof(float);

	int graphicsShapeIndex = -1;

	if (radius>=largeSphereThreshold)
	{
		int numVertices = sizeof(detailed_sphere_vertices)/strideInBytes;
		int numIndices = sizeof(detailed_sphere_indices)/sizeof(int);
		graphicsShapeIndex = m_instancingRenderer->registerShape(&detailed_sphere_vertices[0],numVertices,detailed_sphere_indices,numIndices);
	} else
	{

		if (usePointSprites)
		{
			int numVertices = sizeof(point_sphere_vertices)/strideInBytes;
			int numIndices = sizeof(point_sphere_indices)/sizeof(int);
			graphicsShapeIndex = m_instancingRenderer->registerShape(&point_sphere_vertices[0],numVertices,point_sphere_indices,numIndices,B3_GL_POINTS);
		} else
		{
			if (radius>=mediumSphereThreshold)
			{
				int numVertices = sizeof(medium_sphere_vertices)/strideInBytes;
				int numIndices = sizeof(medium_sphere_indices)/sizeof(int);
				graphicsShapeIndex = m_instancingRenderer->registerShape(&medium_sphere_vertices[0],numVertices,medium_sphere_indices,numIndices);
			} else
			{
				int numVertices = sizeof(low_sphere_vertices)/strideInBytes;
				int numIndices = sizeof(low_sphere_indices)/sizeof(int);
				graphicsShapeIndex = m_instancingRenderer->registerShape(&low_sphere_vertices[0],numVertices,low_sphere_indices,numIndices);
			}
		}
	}
	return graphicsShapeIndex;
}


void SimpleOpenGL3App::drawGrid(DrawGridData data)
{
    int gridSize = data.gridSize;
    float upOffset = data.upOffset;
    int upAxis = data.upAxis;
    float gridColor[4];
    gridColor[0] = data.gridColor[0];
    gridColor[1] = data.gridColor[1];
    gridColor[2] = data.gridColor[2];
    gridColor[3] = data.gridColor[3];

	int sideAxis=-1;
	int forwardAxis=-1;

	switch (upAxis)
	{
		case 1:
			forwardAxis=2;
			sideAxis=0;
			break;
		case 2:
			forwardAxis=1;
			sideAxis=0;
			break;
		default:
			b3Assert(0);
	};
	//b3Vector3 gridColor = b3MakeVector3(0.5,0.5,0.5);

	b3AlignedObjectArray<unsigned int> indices;
	b3AlignedObjectArray<b3Vector3> vertices;
	int lineIndex=0;
	for(int i=-gridSize;i<=gridSize;i++)
	{
		{
			b3Assert(glGetError() ==GL_NO_ERROR);
			b3Vector3 from = b3MakeVector3(0,0,0);
			from[sideAxis] = float(i);
			from[upAxis] = upOffset;
			from[forwardAxis] = float(-gridSize);
			b3Vector3 to=b3MakeVector3(0,0,0);
			to[sideAxis] = float(i);
			to[upAxis] = upOffset;
			to[forwardAxis] = float(gridSize);
			vertices.push_back(from);
			indices.push_back(lineIndex++);
			vertices.push_back(to);
			indices.push_back(lineIndex++);
			// m_instancingRenderer->drawLine(from,to,gridColor);
		}

		b3Assert(glGetError() ==GL_NO_ERROR);
		{

			b3Assert(glGetError() ==GL_NO_ERROR);
			b3Vector3 from=b3MakeVector3(0,0,0);
			from[sideAxis] = float(-gridSize);
			from[upAxis] = upOffset;
			from[forwardAxis] = float(i);
			b3Vector3 to=b3MakeVector3(0,0,0);
			to[sideAxis] = float(gridSize);
			to[upAxis] = upOffset;
			to[forwardAxis] = float(i);
			vertices.push_back(from);
			indices.push_back(lineIndex++);
			vertices.push_back(to);
			indices.push_back(lineIndex++);
			// m_instancingRenderer->drawLine(from,to,gridColor);
		}

	}


	m_instancingRenderer->drawLines(&vertices[0].x,
			gridColor,
			vertices.size(),sizeof(b3Vector3),&indices[0],indices.size(),1);
	

	m_instancingRenderer->drawLine(b3MakeVector3(0,0,0),b3MakeVector3(1,0,0),b3MakeVector3(1,0,0),3);
	m_instancingRenderer->drawLine(b3MakeVector3(0,0,0),b3MakeVector3(0,1,0),b3MakeVector3(0,1,0),3);
	m_instancingRenderer->drawLine(b3MakeVector3(0,0,0),b3MakeVector3(0,0,1),b3MakeVector3(0,0,1),3);

//	void GLInstancingRenderer::drawPoints(const float* positions, const float color[4], int numPoints, int pointStrideInBytes, float pointDrawSize)

	//we don't use drawPoints because all points would have the same color
//	b3Vector3 points[3] = { b3MakeVector3(1, 0, 0), b3MakeVector3(0, 1, 0), b3MakeVector3(0, 0, 1) };
//	m_instancingRenderer->drawPoints(&points[0].x, b3MakeVector3(1, 0, 0), 3, sizeof(b3Vector3), 6);

	m_instancingRenderer->drawPoint(b3MakeVector3(1,0,0),b3MakeVector3(1,0,0),6);
	m_instancingRenderer->drawPoint(b3MakeVector3(0,1,0),b3MakeVector3(0,1,0),6);
	m_instancingRenderer->drawPoint(b3MakeVector3(0,0,1),b3MakeVector3(0,0,1),6);
}

SimpleOpenGL3App::~SimpleOpenGL3App()
{
	delete m_primRenderer ;

	m_window->closeWindow();
	delete m_window;
	delete m_data ;
}

//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
static void writeTextureToFile(int textureWidth, int textureHeight, const char* fileName, FILE* ffmpegVideo)
{
	int numComponents = 4;
	//glPixelStorei(GL_PACK_ALIGNMENT,1);
	
	b3Assert(glGetError()==GL_NO_ERROR);
	//glReadBuffer(GL_BACK);//COLOR_ATTACHMENT0);
	
	float* orgPixels = (float*)malloc(textureWidth*textureHeight*numComponents*4);
	glReadPixels(0,0,textureWidth, textureHeight, GL_RGBA, GL_FLOAT, orgPixels);
	//it is useful to have the actual float values for debugging purposes

	//convert float->char
	char* pixels = (char*)malloc(textureWidth*textureHeight*numComponents);
	assert(glGetError()==GL_NO_ERROR);

	for (int j=0;j<textureHeight;j++)
	{
		for (int i=0;i<textureWidth;i++)
		{
			pixels[(j*textureWidth+i)*numComponents] = char(orgPixels[(j*textureWidth+i)*numComponents]*255.f);
			pixels[(j*textureWidth+i)*numComponents+1]=char(orgPixels[(j*textureWidth+i)*numComponents+1]*255.f);
			pixels[(j*textureWidth+i)*numComponents+2]=char(orgPixels[(j*textureWidth+i)*numComponents+2]*255.f);
			pixels[(j*textureWidth+i)*numComponents+3]=char(orgPixels[(j*textureWidth+i)*numComponents+3]*255.f);
		}
	}



    if (ffmpegVideo)
    {
        fwrite(pixels, textureWidth*textureHeight*numComponents, 1, ffmpegVideo);
        //fwrite(pixels, 100,1,ffmpegVideo);//textureWidth*textureHeight*numComponents, 1, ffmpegVideo);
    } else
    {
        if (1)
        {
            //swap the pixels
            unsigned char tmp;

            for (int j=0;j<textureHeight/2;j++)
            {
                for (int i=0;i<textureWidth;i++)
                {
                    for (int c=0;c<numComponents;c++)
                    {
                        tmp = pixels[(j*textureWidth+i)*numComponents+c];
                        pixels[(j*textureWidth+i)*numComponents+c]=
                        pixels[((textureHeight-j-1)*textureWidth+i)*numComponents+c];
                        pixels[((textureHeight-j-1)*textureWidth+i)*numComponents+c] = tmp;
                    }
                }
            }
        }
        stbi_write_png(fileName, textureWidth,textureHeight, numComponents, pixels, textureWidth*numComponents);
    }


	free(pixels);
	free(orgPixels);

}


void SimpleOpenGL3App::swapBuffer()
{
	m_window->endRendering();
	if (m_data->m_frameDumpPngFileName)
    {
        writeTextureToFile((int)m_window->getRetinaScale()*m_instancingRenderer->getScreenWidth(),
                          (int) m_window->getRetinaScale()*this->m_instancingRenderer->getScreenHeight(),m_data->m_frameDumpPngFileName,
                          m_data->m_ffmpegFile);
        //m_data->m_renderTexture->disable();
        //if (m_data->m_ffmpegFile==0)
        //{
        m_data->m_frameDumpPngFileName = 0;
        //}
    }
	m_window->startRendering();
}

// see also http://blog.mmacklin.com/2013/06/11/real-time-video-capture-with-ffmpeg/
void SimpleOpenGL3App::dumpFramesToVideo(const char* mp4FileName)
{
    int width = (int)m_window->getRetinaScale()*m_instancingRenderer->getScreenWidth();
    int height = (int)m_window->getRetinaScale()*m_instancingRenderer->getScreenHeight();
    char cmd[8192];

#ifdef _WIN32
    sprintf(cmd, "ffmpeg -r 60 -f rawvideo -pix_fmt rgba   -s %dx%d -i - "
    		"-y -crf 0  -b:v 1500000 -an -vcodec h264 -vf vflip  %s", width, height, mp4FileName);
#else
   
   sprintf(cmd, "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s %dx%d -i - "
    		"-threads 0 -y -crf 0 -b 50000k -vf vflip %s", width, height, mp4FileName);
#endif
    
    //sprintf(cmd,"ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s %dx%d -i - "
     //            "-threads 0 -y -crf 0 -b 50000k -vf vflip %s",width,height,mp4FileName);

    //              sprintf(cmd,"ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s %dx%d -i - "
    //              "-threads 0 -preset fast -y -crf 21 -vf vflip %s",width,height,mp4FileName);

    if (m_data->m_ffmpegFile)
    {
        pclose(m_data->m_ffmpegFile);
    }
	if (mp4FileName)
	{
		m_data->m_ffmpegFile = popen(cmd, "w");

		m_data->m_frameDumpPngFileName = mp4FileName;
	}
}
void SimpleOpenGL3App::dumpNextFrameToPng(const char* filename)
{

    // open pipe to ffmpeg's stdin in binary write mode

    m_data->m_frameDumpPngFileName = filename;

//you could use m_renderTexture to allow to render at higher resolutions, such as 4k or so
    if (!m_data->m_renderTexture)
    {
            m_data->m_renderTexture = new GLRenderToTexture();
            GLuint renderTextureId;
            glGenTextures(1, &renderTextureId);

            // "Bind" the newly created texture : all future texture functions will modify this texture
            glBindTexture(GL_TEXTURE_2D, renderTextureId);

            // Give an empty image to OpenGL ( the last "0" )
            //glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, g_OpenGLWidth,g_OpenGLHeight, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
            //glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, g_OpenGLWidth,g_OpenGLHeight, 0,GL_RGBA, GL_FLOAT, 0);
            glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F,
                         m_instancingRenderer->getScreenWidth(),m_instancingRenderer->getScreenHeight()
                         , 0,GL_RGBA, GL_FLOAT, 0);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            m_data->m_renderTexture->init(m_instancingRenderer->getScreenWidth(),this->m_instancingRenderer->getScreenHeight(),renderTextureId, RENDERTEXTURE_COLOR);
    }

    m_data->m_renderTexture->enable();

}

void SimpleOpenGL3App::setUpAxis(int axis)
{
	b3Assert((axis == 1)||(axis==2));//only Y or Z is supported at the moment
	m_data->m_upAxis = axis;
}
int SimpleOpenGL3App::getUpAxis() const
{
	return m_data->m_upAxis;
}
#endif//#ifndef NO_OPENGL3

