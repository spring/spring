/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "AdvSky.h"

#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/TimeProfiler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"

#define Y_PART 10.0
#define X_PART 10.0

#define CLOUD_DETAIL 6
#define CLOUD_MASK (CLOUD_SIZE-1)


CAdvSky::CAdvSky()
	: skydir1(ZeroVector)
	, skydir2(ZeroVector)
	, cdtex(0)
	, cloudFP(0)
	, drawFlare(false)
	, cloudTexMem(NULL)
	, skyTex(0)
	, skyDot3Tex(0)
	, cloudDot3Tex(0)
	, sunTex(0)
	, sunFlareTex(0)
	, skyTexUpdateIter(0)
	, skyDomeList(0)
	, sunFlareList(0)
	, skyAngle(0.0f)
	, domeheight(0.0f)
	, domeWidth(0.0f)
	, sunTexCoordX(0.0f)
	, sunTexCoordY(0.0f)
	, randMatrix(NULL)
	, rawClouds(NULL)
	, blendMatrix(NULL)
	, cloudThickness(NULL)
	, oldCoverBaseX(-5)
	, oldCoverBaseY(0)
	, updatecounter(0)
{
	skytexpart = new unsigned char[512][4];

	if (!(FBO::IsSupported() && GLEW_ARB_fragment_program && ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/clouds.fp"))) {
		throw content_error("ADVSKY: missing OpenGL features!");
	}

	randMatrix = newmat3<int>(16, 32, 32);
	rawClouds = newmat2<int>(CLOUD_SIZE, CLOUD_SIZE);
	blendMatrix = newmat3<int>(CLOUD_DETAIL, 32, 32);

	memset(alphaTransform, 0, 1024);
	memset(thicknessTransform, 0, 1024);
	memset(covers, 0, 4 * 32 * sizeof(float));

	domeheight = std::cos(PI / 16) * 1.01f;
	domeWidth = std::sin(2 * PI / 32) * 400 * 1.7f;

	UpdateSkyDir();
	InitSun();
	UpdateSunDir();

	for (int a = 0; a < CLOUD_DETAIL; a++)
		cloudDown[a] = false;
	for (int a = 0; a < 5; a++)
		cloudDetailDown[a] = false;

	if (fogStart > 0.99f) globalRendering->drawFog = false;

	dynamicSky = true;
	CreateClouds();
	dynamicSky = configHandler->GetBool("DynamicSky");

	cloudFP = LoadFragmentProgram("ARB/clouds.fp");

	CreateSkyDomeList();
}

void CAdvSky::CreateSkyDomeList()
{
	glGetError();
	skyDomeList = glGenLists(1);
	glNewList(skyDomeList, GL_COMPILE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glFogf(GL_FOG_START,-20);
	glFogf(GL_FOG_END,50);
	glFogf(GL_FOG_DENSITY,1.0f/15);
	glEnable(GL_FOG);
	glColor4f(1,1,1,1);

	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, cloudFP );
	glEnable( GL_FRAGMENT_PROGRAM_ARB );
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, sunColor.x,sunColor.y,sunColor.z,0);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, sunColor.x*cloudColor.x,sunColor.y*cloudColor.y,sunColor.z*cloudColor.z,0);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,12, cloudColor.x,cloudColor.y,cloudColor.z,0);

	glBindTexture(GL_TEXTURE_2D, skyTex);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
	glActiveTextureARB(GL_TEXTURE3_ARB);
	glBindTexture(GL_TEXTURE_2D, cdtex);
	glActiveTextureARB(GL_TEXTURE0_ARB);

	for(int y=0;y<Y_PART;y++){
		for(int x=0;x<X_PART;x++){
			glBegin(GL_TRIANGLE_STRIP);
			float3 c=GetCoord(x,y);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			c=GetCoord(x,y+1);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			c=GetCoord(x+1,y);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			c=GetCoord(x+1,y+1);

			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,c.x/domeWidth+0.5f,c.z/domeWidth+0.5f);
			glMultiTexCoord2fARB(GL_TEXTURE2_ARB,c.x*0.025f,c.z*0.025f);
			glMultiTexCoord2fARB(GL_TEXTURE3_ARB,c.x,c.z);
			glVertex3f(c.x,c.y,c.z);

			glEnd();
		}
	}

	glDisable( GL_FRAGMENT_PROGRAM_ARB );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);
	glEndList();
}

CAdvSky::~CAdvSky()
{
	glDeleteTextures(1, &skyTex);
	glDeleteTextures(1, &skyDot3Tex);
	glDeleteTextures(1, &cloudDot3Tex);
	glDeleteLists(skyDomeList, 1);

	glDeleteTextures(1, &sunTex);
	glDeleteTextures(1, &sunFlareTex);
	glDeleteLists(sunFlareList,1);

	delete[] cloudThickness;
	delete[] cloudTexMem;

	glSafeDeleteProgram( cloudFP );

	delmat3<int>(randMatrix);
	delmat2<int>(rawClouds);
	delmat3<int>(blendMatrix);

	delete[] skytexpart;
}

void CAdvSky::Draw()
{
	SCOPED_GMARKER("CAdvSky::Draw");

	if (!globalRendering->drawSky)
		return;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glPushMatrix();
	// glTranslatef3(camera->GetPos());
	glMultMatrixf(CMatrix44f(camera->GetPos(), skydir1, UpVector, skydir2));

	float3 modCamera=skydir1*camera->GetPos().x+skydir2*camera->GetPos().z;

	glMatrixMode(GL_TEXTURE);
	  glActiveTextureARB(GL_TEXTURE2_ARB);
		glPushMatrix();
		glTranslatef((gs->frameNum%20000)*0.00005f+modCamera.x*0.000025f,modCamera.z*0.000025f,0);
	  glActiveTextureARB(GL_TEXTURE3_ARB);
		glPushMatrix();
		glTranslatef((gs->frameNum%20000)*0.0020f+modCamera.x*0.001f,modCamera.z*0.001f,0);
	  glActiveTextureARB(GL_TEXTURE0_ARB);
	glMatrixMode(GL_MODELVIEW);

	glCallList(skyDomeList);

	glMatrixMode(GL_TEXTURE);
	  glActiveTextureARB(GL_TEXTURE2_ARB);
		glPopMatrix();
	  glActiveTextureARB(GL_TEXTURE3_ARB);
		glPopMatrix();
	  glActiveTextureARB(GL_TEXTURE0_ARB);
	glMatrixMode(GL_MODELVIEW);

	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	sky->SetupFog();
}

float3 CAdvSky::GetCoord(int x, int y)
{
	float fy = ((float)y/Y_PART) * 2 * PI;
	float fx = ((float)x/X_PART) * 2 * PI;

	return float3(
		fastmath::sin(fy/32) * fastmath::sin(fx),
		fastmath::cos(fy/32) - domeheight,
		fastmath::sin(fy/32) * fastmath::cos(fx)
	) * 400;
}

void CAdvSky::CreateClouds()
{
	cloudThickness=new unsigned char[CLOUD_SIZE*CLOUD_SIZE+1];
	cloudTexMem=new unsigned char[CLOUD_SIZE*CLOUD_SIZE*4];

	glGenTextures(1, &skyTex);
	glGenTextures(1, &skyDot3Tex);
	glGenTextures(1, &cloudDot3Tex);

	unsigned char (* skytex)[512][4] = new unsigned char[512][512][4]; // this is too big for the stack

	glGenTextures(1, &cdtex);
	glBindTexture(GL_TEXTURE_2D, cdtex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	unsigned char randDetailMatrix[32*32];
	glGenTextures(12, detailTextures);
	for (int a=0; a<6; ++a) {
		int size = std::min(32,256>>a);
		CreateRandDetailMatrix(randDetailMatrix,size);
		glBindTexture(GL_TEXTURE_2D, detailTextures[a]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE, size, size,0,GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);

		CreateRandDetailMatrix(randDetailMatrix,size);
		glBindTexture(GL_TEXTURE_2D, detailTextures[a+6]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE, size, size,0,GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);
	}

	for (int y=0; y<512 ;y++) {
		for(int x=0; x<512; x++) {
			UpdateTexPart(x, y, skytex[y]);
		}
	}

	glBindTexture(GL_TEXTURE_2D, skyTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
//	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, skytex[0][0]);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, skytex[0][0]);
	delete [] skytex;

	unsigned char (* skytex2)[256][4] = new unsigned char[256][256][4];
	for(int y=0;y<256;y++){
		for(int x=0;x<256;x++){
			UpdateTexPartDot3(x, y, skytex2[y]);
		}
	}

	glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR/*_MIPMAP_NEAREST*/);
//	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,256, 256, GL_RGBA, GL_UNSIGNED_BYTE, skytex2[0][0]);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, skytex2[0][0]);
	delete [] skytex2;

	for(int a=0;a<CLOUD_DETAIL;a++){
		CreateRandMatrix(randMatrix[a],1-a*0.03f);
		CreateRandMatrix(randMatrix[a+8],1-a*0.03f);
	}

	glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, CLOUD_SIZE, CLOUD_SIZE,0,GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	fbo.reloadOnAltTab = true;
	fbo.Bind();
	fbo.AttachTexture(cdtex);
	bool status = fbo.CheckStatus("ADVSKY");
	FBO::Unbind();
	if (!status) {
		throw content_error("ADVSKY: FBO is unavailable");
	}

	CreateTransformVectors();
	for(int i=0; i<CLOUD_DETAIL+7; ++i)
		Update();
}

inline void CAdvSky::UpdatePart(int ast, int aed, int a3cstart, int a4cstart) {
	int* rc = *rawClouds + ast;
	unsigned char* ct = cloudTexMem + 4 * ast;

	int yam2 = ydif[(ast - 2) & CLOUD_MASK];
	int yam1 = ydif[(ast - 1) & CLOUD_MASK];
	int yaa  = ydif[(ast)     & CLOUD_MASK];
	int ap1  = (ast + 1) & CLOUD_MASK;

	int a3c  = ast + a3cstart;
	int a4c  = ast + a4cstart;

	for (int a = ast; a < aed; ++rc, ++ct) {
		int yap1 = ydif[ap1] += (int) cloudThickness[++a3c] - cloudThickness[++a] * 2 + cloudThickness[++a4c];

		ap1++;
		ap1 = (ap1 & CLOUD_MASK);
		int dif =
			(yam2 >> 2) +
			(yam1 >> 1) +
			(yaa) +
			(yap1 >> 1) +
			(ydif[ap1] >> 2);
		dif >>= 4;

		yam2 = yam1;
		yam1 = yaa;
		yaa  = yap1;

		*ct++ = 128 + dif;
		*ct++ = thicknessTransform[(*rc) >> 7];
		*ct++ = 255;
	}
}

void CAdvSky::Update()
{
	if (!dynamicSky)
		return;

	SCOPED_TIMER("AdvSky::Update");

	CreateDetailTex();

	static int kernel[CLOUD_SIZE/4*CLOUD_SIZE/4];

	int updatepart = updatecounter++ % (CLOUD_DETAIL + 7);

	switch(updatepart) { // smoothen out the workload across draw frames
	case 0: {
		for(int a=0; a<CLOUD_DETAIL; a++) {
			float fade = gs->frameNum / (70.0f * (2<<(CLOUD_DETAIL-1-a)));
			fade -= std::floor(fade/2)*2;
			if(fade>1) {
				fade = 2 - fade;
				if(!cloudDown[a]) {
					cloudDown[a]=true;
					CreateRandMatrix(randMatrix[a+8],1-a*0.03f);
				}
			} else {
				if(cloudDown[a]) {
					cloudDown[a]=false;
					CreateRandMatrix(randMatrix[a],1-a*0.03f);
				}
			}
			int ifade=(int)(fade*fade*(3-2*fade)*256);
			int ifade2=256-ifade;

			for(int y=0;y<32;y++){
				for(int x=0;x<32;x++)
					blendMatrix[a][y][x]=(randMatrix[a][y][x]*ifade+randMatrix[a+8][y][x]*ifade2)>>8;
			}
		}

		for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++)
			rawClouds[0][a]=0;
		break;
	}
	default: { // 0<updatepart<=CLOUD_DETAIL
		int a=updatepart-1;
		int cs4a=(CLOUD_SIZE/4)>>a;
		int cs8a=(CLOUD_SIZE/8)>>a;
		int cmcs8a=CLOUD_SIZE-cs8a;
		int qcda=(4<<CLOUD_DETAIL)>>a;
		int *pkernel=kernel;
		for(int y=0; y<cs4a; ++y, pkernel+=CLOUD_SIZE/4) {
			float ydist=std::fabs(1.0f+y-cs8a)/cs8a;
			ydist=ydist*ydist*(3-2*ydist);
			int *pkrn=pkernel;
			for(int x=0; x<cs4a; ++x) {
				float xdist=std::fabs(1.0f+x-cs8a)/cs8a;
				xdist=xdist*xdist*(3-2*xdist);

				float contrib=(1-xdist)*(1-ydist);
				(*pkrn++)=(int)(contrib*qcda);
			}
		}
		--cs4a; //!
		int **bm=blendMatrix[a];
		for(int y=0, by=0, bx=0, **prc=rawClouds; y<cmcs8a; y+=cs8a, ++by, prc+=cs8a) {
			for(int x=0; x<cmcs8a; x+=cs8a, ++bx) {
				int blend=bm[by&31][bx&31], **prcy=prc, *pkernel=kernel;
				for(int y2=0; y2<cs4a; ++y2, ++prcy, pkernel+=CLOUD_SIZE/4) {
					int *prcx=(*prcy)+x, *pkrn=pkernel; // pkrn = kernel[y2*CLOUD_SIZE/4+x2];
					for(int x2=0; x2<cs4a; ++x2)
						(*prcx++)+=blend*(*pkrn++); // prcx = rawClouds[y2+y][x2+x]
				}
			}
		}
		for(int y=0, by=0, **prc=rawClouds; y<cmcs8a; y+=cs8a, ++by, prc+=cs8a) {
			int blend=bm[by&31][31&31], **prcy=prc, *pkernel=kernel;
			for(int y2=0; y2<cs4a; ++y2, ++prcy, pkernel+=CLOUD_SIZE/4) {
				int *prcx=(*prcy)+cmcs8a, *pkrn=pkernel;
				for(int x2=cmcs8a; x2 < std::min(CLOUD_SIZE, cs4a+cmcs8a); ++x2)
					(*prcx++)+=blend*(*pkrn++); // prcx = rawClouds[y2+y][x2+cmcs8a], x2<CLOUD_SIZE
				prcx-=CLOUD_SIZE;
				for(int x2=std::max(CLOUD_SIZE,cmcs8a); x2<cs4a+cmcs8a; ++x2)
					(*prcx++)+=blend*(*pkrn++); // prcx = rawClouds[y2+y][x2-cs8a], x2>=CLOUD_SIZE
			}
		}
		for(int x=0, bx=0, **prc=rawClouds+cmcs8a; x<cmcs8a; x+=cs8a, ++bx) {
			int blend=bm[31&31][bx&31], **prcy=prc, *pkernel=kernel;
			for(int y2=cmcs8a; y2<cs4a+cmcs8a; ++y2, ++prcy, pkernel+=CLOUD_SIZE/4) {
				int *prcx=(y2<CLOUD_SIZE) ? (*prcy)+x : (*(prcy-CLOUD_SIZE))+x, *pkrn=pkernel;
				for(int x2=0; x2<cs4a; ++x2) // prcx = rawClouds[y2+cmcs8a][x2+x], y2<CLOUD_SIZE
					(*prcx++)+=blend*(*pkrn++); // prcx =  rawClouds[y2-cs8a][x2+x], y2>=CLOUD_SIZE
			}
		}
		break;
	}
	case CLOUD_DETAIL+1: {
		for(int a=0;a<CLOUD_SIZE*CLOUD_SIZE;a++)
			cloudThickness[a]=alphaTransform[rawClouds[0][a]>>7];

		// this one is read in one place, so to avoid reading uninitialized mem ...
		cloudThickness[CLOUD_SIZE*CLOUD_SIZE] = cloudThickness[CLOUD_SIZE*CLOUD_SIZE-1];

		// create the cloud shading
		for (int a = 0; a < CLOUD_SIZE; ++a) {
			ydif[a] =
				(int)cloudThickness[(a+3*CLOUD_SIZE)]
				+    cloudThickness[(a+2*CLOUD_SIZE)]
				+    cloudThickness[(a+1*CLOUD_SIZE)]
				+    cloudThickness[(a+0*CLOUD_SIZE)]
				-    cloudThickness[(a+CLOUD_SIZE*(CLOUD_SIZE-1))]
				-    cloudThickness[(a+CLOUD_SIZE*(CLOUD_SIZE-2))]
				-    cloudThickness[(a+CLOUD_SIZE*(CLOUD_SIZE-3))];
		}

		ydif[0] += cloudThickness[0+CLOUD_SIZE*(CLOUD_SIZE-3)] - cloudThickness[0]*2 + cloudThickness[0+4*CLOUD_SIZE];
		break;
	}
	case CLOUD_DETAIL+2: {
		UpdatePart(0, CLOUD_SIZE*3-1, CLOUD_SIZE*(CLOUD_SIZE-3), 4*CLOUD_SIZE);
		break;
	}
	case CLOUD_DETAIL+3: {
		UpdatePart(CLOUD_SIZE*3-1, CLOUD_SIZE*(CLOUD_SIZE-4)-1, -3*CLOUD_SIZE, 4*CLOUD_SIZE);
		break;
	}
	case CLOUD_DETAIL+4: {
		UpdatePart(CLOUD_SIZE*(CLOUD_SIZE-4)-1, CLOUD_SIZE*CLOUD_SIZE, -3*CLOUD_SIZE, CLOUD_SIZE*(4-CLOUD_SIZE));
		break;
	}
	case CLOUD_DETAIL+5: {
		const int modDensity = (int) ((1-cloudDensity)*256);
		for (int a = 0; a < CLOUD_SIZE*CLOUD_SIZE; ++a) {
			const int f = (rawClouds[0][a]>>8)-modDensity;
			cloudTexMem[a*4+3]=std::max(0, std::min(255, f));
		}
		break;
	}
	case CLOUD_DETAIL+6: {
		glBindTexture(GL_TEXTURE_2D, cloudDot3Tex);
		glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,CLOUD_SIZE, CLOUD_SIZE,GL_RGBA, GL_UNSIGNED_BYTE, cloudTexMem);
		break;
	}
	}
}

void CAdvSky::CreateRandMatrix(int **matrix,float mod)
{
	for(int a=0, *pmat=*matrix; a<32*32; ++a) {
		float r = ((float)( rand() )) / (float)RAND_MAX;
		*pmat++=((int)(r * 255.0f));
	}
}

void CAdvSky::CreateRandDetailMatrix(unsigned char* matrix,int size)
{
	for(int a=0;a<size*size;a++){
		float  r = ((float)( rand() )) / (float)RAND_MAX;
		*matrix++=((int)(r * 255.0f));
	}
}

void CAdvSky::CreateTransformVectors()
{
	unsigned char *at=alphaTransform;
	unsigned char *tt=thicknessTransform;
	for(int a=0;a<1024;++a){
		float f=(1023.0f-(a+cloudDensity*1024-512))/1023.0f;
		float alpha=std::pow(f*2,3);
		if(alpha>1)
			alpha=1;
		*at=(int) (alpha*255);

		const float d = std::min(1.0f, f * 2.0f);
		*tt++=(unsigned char) (128+d*64+(*at++)*255/(4*255));
	}
}

void CAdvSky::DrawSun()
{
	SCOPED_GMARKER("CAdvSky::DrawSun");

	if (!globalRendering->drawSky)
		return;
	if (!SunVisible(camera->GetPos()))
		return;

	const float3 xzSunCameraPos =
		sundir1 * camera->GetPos().x +
		sundir2 * camera->GetPos().z;
	const float3 modSunColor = sunColor * skyLight->GetLightIntensity();

	// sun-disc vertices might be clipped against the
	// near-plane (which is variable) without scaling
	glPushMatrix();
	glTranslatef3(camera->GetPos());
	glScalef(globalRendering->zNear + 1.0f, globalRendering->zNear + 1.0f, globalRendering->zNear + 1.0f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);

	static unsigned char buf[128];

	const float ymod = (sunTexCoordY - 0.5f) * domeWidth * 0.025f * 256;
	const float fx = gs->frameNum * 0.00005f * CLOUD_SIZE + xzSunCameraPos.x * CLOUD_SIZE * 0.000025f;
	const float fy = ymod + xzSunCameraPos.z * CLOUD_SIZE * 0.000025f;
	const float ffx = fx - std::floor(fx);
	const float ffy = fy - std::floor(fy);
	const int baseX = int(fx) & CLOUD_MASK;
	const int baseY = int(fy) & CLOUD_MASK;

	float
		*cvs0 = (float*) covers[0],
		*cvs1 = (float*) covers[1],
		*cvs2 = (float*) covers[2],
		*cvs3 = (float*) covers[3];

	if (baseX != oldCoverBaseX || baseY != oldCoverBaseY) {
		oldCoverBaseX = baseX;
		oldCoverBaseY = baseY;

		CreateCover(baseX,     baseY,     cvs0);
		CreateCover(baseX + 1, baseY,     cvs1);
		CreateCover(baseX,     baseY + 1, cvs2);
		CreateCover(baseX + 1, baseY + 1, cvs3);
	}

	float mid = 0.0f;
	unsigned char
		*bf1 = buf + 32,
		*bf2 = buf + 64;

	for (int x = 0; x < 32; ++x) {
		const float cx1 =(*cvs0++) * (1.0f - ffx) + (*cvs1++) * ffx;
		const float cx2 =(*cvs2++) * (1.0f - ffx) + (*cvs3++) * ffx;
		const float cover = std::min(127.5f, cx1 * (1.0f - ffy) + cx2 * ffy);

		mid += cover;

		(*bf1++) = (unsigned char) (255 - cover * 2.0f);
		(*bf2++) = (unsigned char) (128 - cover       );
	}

	for (int x = 0; x < 32; ++x) {
		buf[x] = (unsigned char)(255 - (mid * 0.03125f) * 2.0f);
	}

	glBindTexture(GL_TEXTURE_2D, sunFlareTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 32, 3, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);

	glColor4f(modSunColor.x, modSunColor.y, modSunColor.z, 0.0f);
	glCallList(sunFlareList);

	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
}

void CAdvSky::UpdateSunFlare() {
	if (sunFlareList != 0)
		glDeleteLists(sunFlareList, 1);

	const float3 zdir = skyLight->GetLightDir();
	const float3 xdir = zdir.cross(UpVector).ANormalize();
	const float3 ydir = zdir.cross(xdir);

	sunFlareList = glGenLists(1);
	glNewList(sunFlareList, GL_COMPILE);
		glDisable(GL_FOG);
		glBindTexture(GL_TEXTURE_2D, sunFlareTex);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
		glBegin(GL_TRIANGLE_STRIP);

		for (int x = 0; x < 257; ++x) {
			const float dx = std::sin(x * 2.0f * PI / 256.0f);
			const float dy = std::cos(x * 2.0f * PI / 256.0f);
			const float dz = 5.0f;

			glTexCoord2f(x / 256.0f, 0.125f); glVertexf3(zdir * dz + xdir * dx * 0.0014f + ydir * dy * 0.0014f);
			glTexCoord2f(x / 256.0f, 0.875f); glVertexf3(zdir * dz + xdir * dx * 1.0000f + ydir * dy * 1.0000f);
		}

		glEnd();

		if (globalRendering->drawFog)
			glEnable(GL_FOG);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glEndList();
}



void CAdvSky::InitSun()
{
	unsigned char* mem = new unsigned char[128*128*4];

	for(int y=0;y<128;++y){
		for(int x=0;x<128;++x){
			mem[(y*128+x)*4+0]=255;
			mem[(y*128+x)*4+1]=255;
			mem[(y*128+x)*4+2]=255;
			float dist=std::sqrt((float)(y-64)*(y-64)+(x-64)*(x-64));
			if(dist>60)
				mem[(y*128+x)*4+3]=0;
			else
				mem[(y*128+x)*4+3]=255;
		}
	}

	glGenTextures(1, &sunTex);
	glBindTexture(GL_TEXTURE_2D, sunTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,128, 128, GL_RGBA, GL_UNSIGNED_BYTE, mem);

	for(int y=0;y<4;++y){
		for(int x=0;x<32;++x){
			if(y==0 && x%2)
				mem[(y*32+x)]=255;
			else
				mem[(y*32+x)]=0;
		}
	}

	glGenTextures(1, &sunFlareTex);
	glBindTexture(GL_TEXTURE_2D, sunFlareTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
//	gluBuild2DMipmaps(GL_TEXTURE_2D,1 ,32, 2, GL_ALPHA, GL_UNSIGNED_BYTE, mem);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE ,32, 4, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mem);

	delete [] mem;
}

void CAdvSky::CreateCover(int baseX, int baseY, float *buf) const
{
	static int line[]={
		5, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5,
		5, 0, 1, 0, 2, 1, 3, 1, 4, 1, 5,
		5, 0, 1, 1, 2, 1, 3, 2, 4, 2, 5,
		4, 1, 1, 2, 2, 2, 3, 3, 4,
		4, 1, 1, 2, 2, 3, 3, 4, 4,
		4, 1, 1, 2, 2, 3, 2, 4, 3,
		5, 1, 0, 2, 1, 3, 1, 4, 2, 5, 2,
		5, 1, 0, 2, 0, 3, 1, 4, 1, 5, 1
	};

	int *pline=line;

	for(int l=0;l<8;++l){
		int num=*pline++;
		int cover1=0;
		int cover2=0;
		int cover3=0;
		int cover4=0;
		float total=0;
		for(int x=0;x<num;++x){
			int dx=*pline++;
			int dy=*pline++;
			int incy = ((baseY+dy) & CLOUD_MASK) * CLOUD_SIZE;
			int decy = ((baseY-dy) & CLOUD_MASK) * CLOUD_SIZE;
			int incx = (baseX+dx) & CLOUD_MASK;
			int decx = (baseX-dx) & CLOUD_MASK;

			cover1+=255-cloudThickness[incy+decx];//*(num-x);
			cover2+=255-cloudThickness[decy+decx];//*(num-x);
			cover3+=255-cloudThickness[decy+incx];//*(num-x);
			cover4+=255-cloudThickness[incy+incx];//*(num-x);
			total+=1;//(num-x);
		}

		buf[l]=cover1/total;
		buf[l+8]=cover2/total;
		buf[l+16]=cover3/total;
		buf[l+24]=cover4/total;
	}
}

void CAdvSky::CreateDetailTex()
{
	fbo.Bind(); //! render to cdtex

	glViewport(0,0,256,256);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,1,0,1,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glDisable(GL_DEPTH_TEST);

	static unsigned char randDetailMatrix[32*32];

	for(int a=0;a<5;++a){
		float fade = gs->frameNum / float(30<<a);
		fade -= std::floor(fade/2)*2;
		int size = std::min(32,256>>a);

		if(fade>1){
			fade=2-fade;
			if(!cloudDetailDown[a]){
				cloudDetailDown[a]=true;
				CreateRandDetailMatrix(randDetailMatrix,size);
				glBindTexture(GL_TEXTURE_2D, detailTextures[a+6]);
				glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,size, size, GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);
			}
		} else {
			if(cloudDetailDown[a]){
				cloudDetailDown[a]=false;
				CreateRandDetailMatrix(randDetailMatrix,size);
				glBindTexture(GL_TEXTURE_2D, detailTextures[a]);
				glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,size, size, GL_LUMINANCE, GL_UNSIGNED_BYTE, randDetailMatrix);
			}

		}
		float tSize = std::max(1,8>>a);
		float c = std::pow(2.0f,a)*6/255.0f;
		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->CheckInitSize(4*VA_SIZE_T);
		va->AddVertexQT(ZeroVector,    0,     0);
		va->AddVertexQT(RgtVector, tSize,     0);
		va->AddVertexQT(XYVector,  tSize, tSize);
		va->AddVertexQT(UpVector,      0, tSize);

		float ifade = fade*fade*(3-2*fade);

		glBindTexture(GL_TEXTURE_2D, detailTextures[a+6]);
		glColor4f(c,c,c,1-ifade);
		va->DrawArrayT(GL_QUADS);
		glBindTexture(GL_TEXTURE_2D, detailTextures[a]);
		glColor4f(c,c,c,ifade);
		va->DrawArrayT(GL_QUADS);
	}

	glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
	fbo.Unbind();

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void CAdvSky::UpdateSunDir() {
	const float3& L = skyLight->GetLightDir();

	sundir2 = L;
	sundir2.y = 0.0f;

	if (sundir2.SqLength() == 0.0f)
		sundir2.x = 1.0f;

	sundir2.ANormalize();
	sundir1 = sundir2.cross(UpVector);

	modSunDir.x = 0.0f;
	modSunDir.y = L.y;
	modSunDir.z = std::sqrt(L.x * L.x + L.z * L.z);

	sunTexCoordX = 0.5f;
	sunTexCoordY = GetTexCoordFromDir(modSunDir);

	UpdateSunFlare();
}

void CAdvSky::UpdateSkyDir() {
	skydir2 = mapInfo->atmosphere.skyDir;
	skydir2.y = 0.0f;

	if (skydir2.SqLength() == 0.0f)
		skydir2.x = 1.0f;

	skydir2.ANormalize();
	skydir1 = skydir2.cross(UpVector);
	skyAngle = GetRadFromXY(skydir2.x, skydir2.z) + PI / 2.0f;
}

void CAdvSky::UpdateSkyTexture() {
	const int mod = skyTexUpdateIter % 3;

	if (mod <= 1) {
		const int y = (skyTexUpdateIter / 3) * 2 + mod;
		for (int x = 0; x < 512; x++) {
			UpdateTexPart(x, y, skytexpart);
		}
		glBindTexture(GL_TEXTURE_2D, skyTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, skytexpart[0]);
	} else {
		const int y = (skyTexUpdateIter / 3);
		for (int x = 0; x < 256; x++) {
			UpdateTexPartDot3(x, y, skytexpart);
		}
		glBindTexture(GL_TEXTURE_2D, skyDot3Tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, skytexpart[0]);
	}

	skyTexUpdateIter = (skyTexUpdateIter + 1) % (512 + 256);
}



float3 CAdvSky::GetDirFromTexCoord(float x, float y)
{
	float3 dir;

	dir.x = (x - 0.5f) * domeWidth;
	dir.z = (y - 0.5f) * domeWidth;

	const float hdist = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	const float ang = GetRadFromXY(dir.x, dir.z) + skyAngle;
	const float fy = std::asin(hdist / 400);

	dir.x = hdist * std::cos(ang);
	dir.z = hdist * std::sin(ang);
	dir.y = (std::cos(fy) - domeheight) * 400;

	dir.ANormalize();
	return dir;
}

// should be improved
// only take stuff in yz plane
float CAdvSky::GetTexCoordFromDir(const float3& dir)
{
	float tp = 0.5f;
	float step = 0.25f;

	for (int a = 0; a < 10; ++a) {
		float tx = 0.5f + tp;
		const float3& d = GetDirFromTexCoord(tx, 0.5f);

		if (d.y < dir.y)
			tp -= step;
		else
			tp += step;

		step *= 0.5f;
	}

	return (0.5f + tp);
}

void CAdvSky::UpdateTexPartDot3(int x, int y, unsigned char (*texp)[4]) {
	const float3& dir = GetDirFromTexCoord(x / 256.0f, (255.0f - y) / 256.0f);

	const float sunInt = skyLight->GetLightIntensity();
	const float sunDist = std::acos(dir.dot(skyLight->GetLightDir())) * 50;
	const float sunMod = sunInt * (0.3f / std::sqrt(sunDist) + 3.0f / (1 + sunDist));

	const float green = std::min(1.0f, (0.55f + sunMod));
	const float blue  = 203 - sunInt * (40.0f / (3 + sunDist));

	texp[x][0] = (unsigned char) (sunInt * (255 - std::min(255.0f, sunDist))); // sun on borders
	texp[x][1] = (unsigned char) (green * 255); // sun light through
	texp[x][2] = (unsigned char)  blue; // ambient
	texp[x][3] = 255;
}

void CAdvSky::UpdateTexPart(int x, int y, unsigned char (*texp)[4]) {
	const float3& dir = GetDirFromTexCoord(x / 512.0f, (511.0f - y) / 512.0f);

	const float sunDist = std::acos(dir.dot(skyLight->GetLightDir())) * 70;
	const float sunMod = skyLight->GetLightIntensity() * 12.0f / (12 + sunDist);

	const float red   = std::min(skyColor.x + sunMod * sunColor.x, 1.0f);
	const float green = std::min(skyColor.y + sunMod * sunColor.y, 1.0f);
	const float blue  = std::min(skyColor.z + sunMod * sunColor.z, 1.0f);

	texp[x][0] = (unsigned char)(red   * 255);
	texp[x][1] = (unsigned char)(green * 255);
	texp[x][2] = (unsigned char)(blue  * 255);
	texp[x][3] = 255;
}
