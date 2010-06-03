/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlyingPiece.hpp"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/3DOParser.h"
#include "Rendering/Models/S3OParser.h"
#include "System/GlobalUnsynced.h"
#include "System/Matrix44f.h"

FlyingPiece::~FlyingPiece() {
	if (verts != NULL) {
		delete[] verts;
	}
}

void FlyingPiece::Draw(int modelType, size_t* lastTeam, size_t* lastTex, CVertexArray* va) {
	switch (modelType) {
		case MODELTYPE_3DO: {
			if (team != *lastTeam) {
				*lastTeam = team;

				va->DrawArrayTN(GL_QUADS);
				va->Initialize();
				unitDrawer->SetTeamColour(team);
			}

			CMatrix44f m;
			m.Rotate(rot, rotAxis);
			float3 tp, tn;

			const float3 interPos = pos + speed * globalRendering->timeOffset;
			const C3DOTextureHandler::UnitTexture* tex = prim->texture;

			const std::vector<S3DOVertex>& vertices    = object->vertices;
			const std::vector<int>&        verticesIdx = prim->vertices;

			const S3DOVertex* v = NULL;
			const float uvCoords[8] = {
				tex->xstart, tex->ystart,
				tex->xend,   tex->ystart,
				tex->xend,   tex->yend,
				tex->xstart, tex->yend
			};

			for (int i = 0; i < 4; i++) {
				v = &vertices[verticesIdx[i]];
				tp = m.Mul(v->pos) + interPos;
				tn = m.Mul(v->normal);
				va->AddVertexQTN(tp, uvCoords[i << 1], uvCoords[(i << 1) + 1], tn);
			}
		} break;

		case MODELTYPE_S3O: {
			if (texture != *lastTex) {
				*lastTex = texture;

				if (*lastTex == 0)
					return;

				va->DrawArrayTN(GL_QUADS);
				va->Initialize();
				texturehandlerS3O->SetS3oTexture(texture);
			}
			if (team != *lastTeam) {
				*lastTeam = team;

				va->DrawArrayTN(GL_QUADS);
				va->Initialize();
				unitDrawer->SetTeamColour(team);
			}

			CMatrix44f m;
			m.Rotate(rot, rotAxis);
			float3 tp, tn;

			const float3 interPos = pos + speed * globalRendering->timeOffset;

			for (int i = 0; i < 4; i++) {
				tp = m.Mul(verts[i].pos) + interPos;
				tn = m.Mul(verts[i].normal);
				va->AddVertexQTN(tp, verts[i].textureX, verts[i].textureY, tn);
			}
		} break;

		default: {
		} break;
	}
}
