/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEIGHT_MAP_TEXTURE_H
#define HEIGHT_MAP_TEXTURE_H

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"

class HeightMapTexture {
	public:
		HeightMapTexture();
		~HeightMapTexture();

		void Init();
		void Kill();

		void UpdateArea(int x0, int z0, int x1, int z1);
		GLuint GetTextureID() const { return texID; }
		inline GLuint CheckTextureID()
		{
			if (texID != 0) {
				return texID;
			}
			else {
				if (init) {
					return 0;
				} else {
					Init();
					return texID;
				}
			}
		}

		int GetSizeX() const { return xSize; }
		int GetSizeY() const { return ySize; }

	private:
		bool init;
		GLuint texID;
		int xSize;
		int ySize;
		PBO pbo;
};

extern HeightMapTexture heightMapTexture;

#endif // HEIGHTMAP_TEXTURE_H

