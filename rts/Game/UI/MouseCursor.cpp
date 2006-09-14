#include "StdAfx.h"
#include "MouseCursor.h"
#include "CommandColors.h"
#include "InfoConsole.h"
#include "FileSystem/FileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "myMath.h"
#include "bitops.h"
#include "mmgr.h"


CMouseCursor* CMouseCursor::New(const string &name, HotSpot hs)
{
	// FIXME: not used, yet
	CMouseCursor* c = new CMouseCursor(name, hs);
	if (c->frames.size() == 0) {
		delete c;
		return NULL;
	}
	return c;	
}


//Would be nice if these were read from a gaf-file instead.
CMouseCursor::CMouseCursor(const string &name, HotSpot hs)
{
	char namebuf[100];
	curFrame = 0;

	if (name.length() > 80) {
		logOutput.Print("CMouseCursor: Long name %s", name.c_str());
		return;
	}

	int maxxsize = 0;
	int maxysize = 0;

	for (int frameNum = 0; ; ++frameNum) {
                sprintf(namebuf, "Anims/%s_%d.bmp", name.c_str(), frameNum);
		CFileHandler f(namebuf);
		
		if (f.FileExists()) {
			string bmpname = namebuf;
			CBitmap b;
			if (!b.Load(bmpname))
				throw content_error("Could not load mouse cursor from file " + bmpname);
			b.ReverseYAxis();
			CBitmap *final = getAlignedBitmap(b);
			setBitmapTransparency(*final, 84, 84, 252); 

			unsigned int cursorTex = 0;
			glGenTextures(1, &cursorTex);
			glBindTexture(GL_TEXTURE_2D, cursorTex);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,final->xsize, final->ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, final->mem);

			FrameData frame(cursorTex, final->xsize, final->ysize);
			frames.push_back(frame);

			maxxsize = max(b.xsize, maxxsize);
			maxysize = max(b.ysize, maxysize);

			delete final;
		}
		else {
			break;
		}
	}

	if (frames.size() == 0) {
		logOutput.Print("No such cursor: %s", name.c_str());
		return;
	}

	switch (hs) {
		case TopLeft:
			xofs = 0; 
			yofs = 0;
			break;
		case Center:
			xofs = maxxsize / 2;
			yofs = maxysize / 2;
			break;
	}

	lastFrameTime = gu->gameTime;
}


CMouseCursor::~CMouseCursor(void)
{
	for (vector<FrameData>::iterator i = frames.begin(); i != frames.end(); ++i) {
		unsigned int tn = i->texture;
		glDeleteTextures(1, &tn);
	}
}


CBitmap* CMouseCursor::getAlignedBitmap(const CBitmap &orig)
{
	CBitmap *nb;

	int nx = next_power_of_2(orig.xsize);
	int ny = next_power_of_2(orig.ysize);

	unsigned char *data = new unsigned char[nx * ny * 4];
	memset(data, 0, nx * ny * 4);

	for (int y = 0; y < orig.ysize; ++y) 
		for (int x = 0; x < orig.xsize; ++x) 
			for (int v = 0; v < 4; ++v) {
				data[((y + (ny-orig.ysize))*nx+x)*4+v] = orig.mem[(y*orig.xsize+x)*4+v];
			}
		
	nb = new CBitmap(data, nx, ny);
	delete[] data;
	return nb;
}


void CMouseCursor::setBitmapTransparency(CBitmap &bm, int r, int g, int b)
{
	for(int y=0;y<bm.ysize;++y){
		for(int x=0;x<bm.xsize;++x){
			if ((bm.mem[(y*bm.xsize+x)*4 + 0] == r) &&
				(bm.mem[(y*bm.xsize+x)*4 + 1] == g) &&
				(bm.mem[(y*bm.xsize+x)*4 + 2] == b)) 
			{
				bm.mem[(y*bm.xsize+x)*4 + 3] = 0;
			}
		}
	}
}


void CMouseCursor::Draw(int x, int y)
{
	if (frames.size()==0)
		return;

	//Center on hotspot
	x -= xofs;
	y -= yofs;

	const FrameData& f = frames[curFrame];
	unsigned int cursorTex = f.texture;
	int xs = f.xsize;
	int ys = f.ysize;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,cursorTex);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER,0.01f);
	glColor4f(1,1,1,1);

	glViewport(x,gu->screeny - y - ys,xs,ys);

	glBegin(GL_QUADS);

 	glTexCoord2f(0,0);glVertex3f(0,0,0);
 	glTexCoord2f(0,1);glVertex3f(0,1,0);
 	glTexCoord2f(1,1);glVertex3f(1,1,0);
 	glTexCoord2f(1,0);glVertex3f(1,0,0);

	glEnd();

/*	glViewport(x+10,gu->screeny-y-30,60*gu->screenx/gu->screeny,60);
	glScalef(0.2f,0.2f,0.2f);
	font->glPrint("%s",cursorText.c_str());

	glViewport(lastx-20,gu->screeny-lasty-30,60*gu->screenx/gu->screeny,60);
	font->glPrint("%s",cursorTextRight.c_str());
	cursorTextRight=""; */

	glViewport(0,0,gu->screenx,gu->screeny);
}


void CMouseCursor::DrawQuad(int x, int y)
{
	if (frames.size()==0) {
		return;
	}

	const float scale = cmdColors.QueueIconScale();

	const FrameData& f = frames[curFrame];
	const int xs = int(float(f.xsize) * scale);  
	const int ys = int(float(f.ysize) * scale);                      

	//Center on hotspot
	const int xp = int(float(x) - (float(xofs) * scale));
	const int yp = int(float(y) - (float(ys) - (float(yofs) * scale)));

	glViewport(xp, yp, xs, ys);

	glBegin(GL_QUADS);
 	glTexCoord2f(0,0);glVertex3f(0,0,0);
 	glTexCoord2f(0,1);glVertex3f(0,1,0);
 	glTexCoord2f(1,1);glVertex3f(1,1,0);
 	glTexCoord2f(1,0);glVertex3f(1,0,0);
	glEnd();
}


void CMouseCursor::Update()
{
	if (frames.size() == 0) {
		return;
	}
	
	//Advance a frame in animated cursors
	if (gu->gameTime - lastFrameTime > 0.1f) {
		lastFrameTime = gu->gameTime;
		curFrame = (curFrame + 1) % (int)frames.size();
	}
}


void CMouseCursor::BindTexture()
{
	if (frames.size() == 0) {
		return;
	}
	unsigned int cursorTex = frames[curFrame].texture;
	glBindTexture(GL_TEXTURE_2D, cursorTex);
}

