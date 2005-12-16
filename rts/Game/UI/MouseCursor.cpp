#include "StdAfx.h"
#include "MouseCursor.h"
#include "FileHandler.h"
#include "Bitmap.h"
#include "InfoConsole.h"
#include "myGL.h"
#include "myMath.h"
//#include "mmgr.h"

//Would be nice if these were read from a gaf-file instead.
CMouseCursor::CMouseCursor(const string &name, HotSpot hs)
{
	char namebuf[100];
	curFrame = 0;



	if (name.length() > 80) {
		info->AddLine("CMouseCursor: Long name %s", name.c_str());
		return;
	}

	int maxxsize = 0;
	int maxysize = 0;

	for (int frameNum = 0; ; ++frameNum) {
                sprintf(namebuf, "Anims/%s_%d.bmp", name.c_str(), frameNum);
		CFileHandler f(namebuf);
		
		if (f.FileExists()) {
			string bmpname = namebuf;
			CBitmap b(bmpname);
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

			frames.push_back(cursorTex);
			xsize.push_back(final->xsize);
			ysize.push_back(final->ysize);

			maxxsize = max(b.xsize, maxxsize);
			maxysize = max(b.ysize, maxysize);

			delete final;
		}
		else {
			break;
		}
	}

	if (frames.size() == 0) {
		info->AddLine("No such cursor: %s", name.c_str());
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
	for (vector<unsigned int>::iterator i = frames.begin(); i != frames.end(); ++i) {
		unsigned int tn = *i;
		glDeleteTextures(1, &tn);
	}
}

CBitmap* CMouseCursor::getAlignedBitmap(const CBitmap &orig)
{
	CBitmap *nb;

	int nx = NextPwr2(orig.xsize);
	int ny = NextPwr2(orig.ysize);

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
	//Advance a frame in animated cursors
	if (gu->gameTime - lastFrameTime > 0.1) {
		lastFrameTime = gu->gameTime;
		curFrame++;
		if (curFrame >= (int)frames.size()) 
			curFrame = 0;
	}

	//Center on hotspot
	x -= xofs;
	y -= yofs;

	unsigned int cursorTex = frames[curFrame];
	int xs = xsize[curFrame];
	int ys = ysize[curFrame];

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,cursorTex);
	if (frames.empty())
		return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER,0.01f);

	glViewport(x,gu->screeny - y - ys,xs,ys);

	glBegin(GL_QUADS);

 	glTexCoord2f(0,0);glVertex3f(0,0,0);
 	glTexCoord2f(0,1);glVertex3f(0,1,0);
 	glTexCoord2f(1,1);glVertex3f(1,1,0);
 	glTexCoord2f(1,0);glVertex3f(1,0,0);

	glEnd();

/*	glViewport(x+10,gu->screeny-y-30,60*gu->screenx/gu->screeny,60);
	glScalef(0.2,0.2,0.2);
	font->glPrint("%s",cursorText.c_str());

	glViewport(lastx-20,gu->screeny-lasty-30,60*gu->screenx/gu->screeny,60);
	font->glPrint("%s",cursorTextRight.c_str());
	cursorTextRight=""; */

	glViewport(0,0,gu->screenx,gu->screeny);
}

