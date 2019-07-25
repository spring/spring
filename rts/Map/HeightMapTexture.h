/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEIGHT_MAP_TEXTURE_H
#define HEIGHT_MAP_TEXTURE_H

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"
#include "System/EventClient.h"

class HeightMapTexture : public CEventClient
{
	public:
		//! CEventClient interface
		bool WantsEvent(const std::string& eventName) {
			return (eventName == "UnsyncedHeightMapUpdate");
		}
		bool GetFullRead() const { return true; }
		int GetReadAllyTeam() const { return AllAccessTeam; }

		void UnsyncedHeightMapUpdate(const SRectangle& rect);

	public:
		HeightMapTexture();
		~HeightMapTexture();

		GLuint GetTextureID() const { return texID; }

		int GetSizeX() const { return xSize; }
		int GetSizeY() const { return ySize; }

	private:
		void Init();
		void Kill();

	private:
		GLuint texID = 0;

		int xSize = 0;
		int ySize = 0;

		PBO pbos[3];
};

extern HeightMapTexture* heightMapTexture;

#endif // HEIGHTMAP_TEXTURE_H

