#include "StdAfx.h"
#include "TerrainBase.h"
#include "Terrain.h"
#include "TerrainNode.h"
#include "Textures.h"
#include "Lightcalc.h"

#include <SDL.h>

namespace terrain {

using namespace std;

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
			nd[y*w+x] = result / 8.0f;
		}
	}
	memcpy(data,nd,w*h);
	delete[] nd;
}

Lightmap::Lightmap(Heightmap *orghm, int level, LightingInfo *li)
{
	int startTicks = SDL_GetTicks();
	tilesize.x = orghm->w-1;
	tilesize.y = orghm->h-1;
	name = "lightmap";

	Heightmap *hm = orghm->GetLevel(-level);
	int w=hm->w-1;
	float org2c = w/float(orghm->w-1);
	float c2org = (float)(orghm->w-1)/w;

	float *centerhm = new float[w*w];
	Vector3 *shading = new Vector3[w*w];
	for (int y=0;y<w;y++)
		for (int x=0;x<w;x++) {
			centerhm[y*w+x] = hm->scale * 0.25f * ( (int)hm->at(x,y)+ (int)hm->at(x+1,y)+ (int)hm->at(x,y+1) + (int)hm->at(x+1,y+1) ) + hm->offset;
			shading[y*w+x] = li->ambient;
		}

	uchar *lightMap = new uchar[w*w];
	int lightIndex = 0;
	for (std::vector<StaticLight>::const_iterator l=li->staticLights.begin();l!=li->staticLights.end();++l) 
	{
		memset(lightMap, 255, w*w); // 255 is lit, 0 is unlit

		int lightx = l->position.x / hm->squareSize;
		int lighty = l->position.z / hm->squareSize;
		
		for (int y=0;y<w;y++)
		{
			for (int x=0;x<w;x++)
			{
				if (!lightMap[y*w+x]) // shadowed pixels can't shadow other pixels
					continue;

				if (x==lightx && y==lighty)
					continue;

				float dx = lightx-x;
				float dy = lighty-y;
				float h = centerhm[y*w+x];

				float dh = l->position.y-h;
				float len = sqrtf(dx*dx+dy*dy);
				const float step = 5.0f;
				float invLength2d = step/len;
				dx *= invLength2d;
				dy *= invLength2d;
				dh *= invLength2d;

				float px = x + dx, py = y + dy;
				h += dh;
				while (px >= 0.0f && px < w && py >= 0.0f && py < w && len >= 0.0f)
				{
					int index = (int)py * w + (int)px;

					if (centerhm[index] > h + 2.0f)
					{
						lightMap[y*w+x]=0;
						break;
					}

					px += dx;
					py += dy;
					h += dh;
					len -= step;
				}
			}
		}

		BlurGrayscaleImage(w,w,lightMap);

		char sm_fn[64];
		SNPRINTF(sm_fn, sizeof(sm_fn), "shadowmap%d.bmp", lightIndex++);
		SaveImage(sm_fn, 1, IL_UNSIGNED_BYTE, w,w, lightMap);

		for (int y=0;y<w;y++)
		{
			for (int x=0;x<w;x++)
			{
				if (!lightMap[y*w+x])
					continue;

				Vector3 wp((x+0.5f)*hm->squareSize,centerhm[y*w+x],(y+0.5f)*hm->squareSize);
				wp = l->position - wp;

				uchar* normal = hm->GetNormal (x,y);
				Vector3 normv((2 * (int)normal[0] - 256)/255.0f, (2 * (int)normal[1] - 256)/255.0f, (2 * (int)normal[2] - 256)/255.0f);

				wp.Normalize();
				float dot = wp.dot(normv);
				if(dot < 0.0f) dot = 0.0f; 
				if(dot > 1.0f) dot = 1.0f;
				dot *= lightMap[y*w+x]*(1.0f/255.0f);
				shading[y*w+x] += l->color * dot;
			}
		}

	}
	delete[] lightMap;

	glGenTextures(1,&shadingTex);
	glBindTexture (GL_TEXTURE_2D, shadingTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	uchar *shadingTexData=new uchar[w*w*3], *td = shadingTexData;
	for(int y=0;y<w;y++) {
		for (int x=0;x<w;x++) {
			shadingTexData[(y*w+x)*3+0] = min(1.0f, shading[y*w+x].x) * 255;
			shadingTexData[(y*w+x)*3+1] = min(1.0f, shading[y*w+x].y) * 255;
			shadingTexData[(y*w+x)*3+2] = min(1.0f, shading[y*w+x].z) * 255;
		}
	}


	SaveImage ("lightmap.png", 3, IL_UNSIGNED_BYTE, w,w, shadingTexData);

	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w,w, GL_RGB, GL_UNSIGNED_BYTE, shadingTexData);
	delete[] shadingTexData;

	id = shadingTex;

	delete[] shading;
	delete[] centerhm;

	int numTicks = SDL_GetTicks() - startTicks;
	d_trace ("Lightmap generation: %2.3f seconds\n", numTicks * 0.001f);
}

Lightmap::~Lightmap()
{
	if (shadingTex)
		glDeleteTextures(1, &shadingTex);
}

};
