/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/
#include "StdAfx.h"
#include <cstring>
#include "TerrainBase.h"
#include "Terrain.h"
#include "TerrainNode.h"
#include "Textures.h"
#include "Lightcalc.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Util.h"

#include <IL/il.h>
#include <assert.h>

#include "System/myTime.h"

namespace terrain {

using std::min;

void BlurGrayscaleImage(int w, int h, uchar *data)
{
	uchar *nd = new uchar [w*h];

	for (int y=0;y<h;y++) {
		for (int x=0;x<w;x++) {
			int l=x?x-1:x;
			int r=x<w-1?x+1:x;
			int t=y?y-1:y;
			int b=y<h-1?y+1:y;

			int result = 0;

#define AT(X,Y) result += data[w*(Y)+(X)]
			AT(l,t);
			AT(x,t);
			AT(r,t);

			AT(l,y);
			AT(r,y);

			AT(l,b);
			AT(x,b);
			AT(r,b);
#undef AT
			nd[y*w+x] = (uchar)(result / 8);
		}
	}
	memcpy(data,nd,w*h);
	delete[] nd;
}

Lightmap::Lightmap(Heightmap *orghm, int level, int shadowLevelDif, LightingInfo *li)
{
	const spring_time startTicks = spring_gettime();
	tilesize.x = orghm->w-1;
	tilesize.y = orghm->h-1;
	name = "lightmap";

	Heightmap *hm;
	int w;

	for(;;) {
		hm = orghm->GetLevel(-level);
		w=hm->w-1;

		GLint maxw;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxw);

		if (w > maxw) level ++;
		else break;
	}

	shadowLevelDif=0;
	Heightmap *shadowhm = orghm->GetLevel(-(level+shadowLevelDif));
	int shadowScale=1<<shadowLevelDif;
	int shadowW=shadowhm->w-1;
	assert (w/shadowW == shadowScale);
	//float org2c = w/float(orghm->w-1);
	//float c2org = (float)(orghm->w-1)/w;

	float *centerhm = new float[w*w];
	Vector3 *shading = new Vector3[w*w];
	for (int y=0;y<w;y++)
		for (int x=0;x<w;x++) {
			centerhm[y*w+x] =/* hm->scale * */ 0.25f * ( (int)hm->at(x,y)+ (int)hm->at(x+1,y)+ (int)hm->at(x,y+1) + (int)hm->at(x+1,y+1) ); //+ hm->offset;
			shading[y*w+x] = li->ambient;
		}

	uchar *lightMap = new uchar[shadowW*shadowW];
	for (std::vector<StaticLight>::const_iterator l=li->staticLights.begin();l!=li->staticLights.end();++l)
	{
		float lightx;
		float lighty;

		if (l->directional) {
			lightx = l->position.x;
			lighty = l->position.y;
		} else {
			lightx = (int)(l->position.x / shadowhm->squareSize);
			lighty = (int)(l->position.z / shadowhm->squareSize);
		}
		CalculateShadows(lightMap, shadowW, lightx, lighty,
			l->position.y, centerhm, w, shadowScale, l->directional);

		for (int y=0;y<w;y++)
		{
			for (int x=0;x<w;x++)
			{
				if (!lightMap[(y*shadowW+x)/shadowScale])
					continue;

				Vector3 wp;
				if (l->directional)
					wp = l->position;
				else
					wp = l->position - Vector3((x+0.5f)*hm->squareSize,centerhm[y*w+x],(y+0.5f)*hm->squareSize);

				uchar* normal = hm->GetNormal (x,y);
				Vector3 normv((2 * (int)normal[0] - 256)/255.0f, (2 * (int)normal[1] - 256)/255.0f, (2 * (int)normal[2] - 256)/255.0f);

				wp.ANormalize();
				float dot = wp.dot(normv);
				if(dot < 0.0f) dot = 0.0f;
				if(dot > 1.0f) dot = 1.0f;
				dot *= lightMap[(y*shadowW+x)/shadowScale]*(1.0f/255.0f);
				shading[y*w+x] += l->color * dot;
			}
		}

	}
	delete[] lightMap;

	glGenTextures(1,&shadingTex);
	glBindTexture (GL_TEXTURE_2D, shadingTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	uchar *shadingTexData=new uchar[w*w*4];
	for(int y=0;y<w;y++) {
		for (int x=0;x<w;x++) {
			shadingTexData[(y*w+x)*4+0] = (uchar)(min(1.0f, shading[y*w+x].x) * 255);
			shadingTexData[(y*w+x)*4+1] = (uchar)(min(1.0f, shading[y*w+x].y) * 255);
			shadingTexData[(y*w+x)*4+2] = (uchar)(min(1.0f, shading[y*w+x].z) * 255);
			shadingTexData[(y*w+x)*4+3] = CReadMap::EncodeHeight(centerhm[w*y+x]);
		}
	}

	SaveImage ("lightmap.png", 4, IL_UNSIGNED_BYTE, w,w, shadingTexData);

	glBuildMipmaps(GL_TEXTURE_2D, 4, w,w, GL_RGBA, GL_UNSIGNED_BYTE, shadingTexData);
	delete[] shadingTexData;

	id = shadingTex;

	delete[] shading;
	delete[] centerhm;

	const spring_duration numTicks = spring_gettime() - startTicks;
	d_trace ("Lightmap generation: %2.3f seconds\n", spring_tomsecs(numTicks) * 0.001f);
}

Lightmap::~Lightmap()
{
	if (shadingTex)
		glDeleteTextures(1, &shadingTex);
}

void Lightmap::CalculateShadows (uchar *dst, int dstw, float lightX,float lightY, float lightH, float *centerhm, int hmw, int hmscale, bool directional)
{
	memset(dst, 255, dstw*dstw); // 255 is lit, 0 is unlit

	for (int y=0;y<dstw;y++)
	{
		for (int x=0;x<dstw;x++)
		{
			if (!dst[y*dstw+x]) // shadowed pixels can't shadow other pixels
				continue;

			float dx,dy,dh;
			float h = centerhm[(y*hmw+x)*hmscale];

			if (directional) {
				dx = lightX;
				dy = lightY;
				dh = lightH;
			} else {
				dx = lightX-x;
				dy = lightY-y;
				dh = lightH-h;

				if (x==lightX && y==lightY)
					continue;
			}

			float len = sqrt(dx*dx+dy*dy);
			const float step = 5.0f;
			float invLength2d = step/len;
			dx *= invLength2d;
			dy *= invLength2d;
			dh *= invLength2d;

			float px = (x + dx) * hmscale, py = (y + dy) * hmscale;
			h += dh;
			while (px >= 0.0f && px < hmw && py >= 0.0f && py < hmw && len >= 0.0f)
			{
				int index = (int)py * hmw + (int)px;

				if (centerhm[index] > h + 2.0f)
				{
					dst[y*dstw+x]=0;
					break;
				}

				px += dx;
				py += dy;
				h += dh;
				len -= step;
			}
		}
	}

	BlurGrayscaleImage(dstw, dstw, dst);

	char sm_fn[64];
	static int lightIndex=0;
	SNPRINTF(sm_fn, sizeof(sm_fn), "shadowmap%d.png", lightIndex++);
	SaveImage(sm_fn, 1, IL_UNSIGNED_BYTE, dstw,dstw, dst);
}
};
