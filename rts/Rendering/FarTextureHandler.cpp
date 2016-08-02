/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FarTextureHandler.h"

#include "Game/Camera.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Objects/SolidObject.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"

#define LOG_SECTION_FAR_TEXTURE_HANDLER "FarTextureHandler"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_FAR_TEXTURE_HANDLER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_FAR_TEXTURE_HANDLER

#define NUM_ICON_ORIENTATIONS 8


CFarTextureHandler* farTextureHandler = nullptr;

CFarTextureHandler::CFarTextureHandler()
{
	farTextureID = 0;
	usedFarTextures = 0;

	// ATI supports 16K textures, but that might be a bit too much
	// for this purpose so limit the atlas to 4K (still sufficient)
	iconSize.x = 32 * 4;
	iconSize.y = 32 * 4;
	texSize.x = std::min(globalRendering->maxTextureSize, 4096);
	texSize.y = std::max(iconSize.y, 4 * NUM_ICON_ORIENTATIONS * iconSize.x * iconSize.y / texSize.x); // minimum space for 4 icons
	texSize.y = next_power_of_2(texSize.y);

	#ifdef HEADLESS
	return;
	#endif

	if (!fbo.IsValid()) {
		LOG_L(L_WARNING, "framebuffer not valid!");
		return;
	}

	glGenTextures(1, &farTextureID);
	glBindTexture(GL_TEXTURE_2D, farTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texSize.x, texSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	fbo.Bind();
	fbo.AttachTexture(farTextureID);

	if (fbo.CheckStatus("FARTEXTURE")) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	fbo.Unbind();

	fbo.reloadOnAltTab = true;
}


CFarTextureHandler::~CFarTextureHandler()
{
	glDeleteTextures(1, &farTextureID);
	drawQueue.clear();
}


/**
 * @brief Returns the (row, column) pair of a FarTexture in the TextureAtlas.
 */
int2 CFarTextureHandler::GetTextureCoordsInt(const int farTextureNum, const int orientation) const
{
	const int texnum = (farTextureNum * NUM_ICON_ORIENTATIONS) + orientation;

	const int row = texnum       / (texSize.x / iconSize.x);
	const int col = texnum - row * (texSize.x / iconSize.x);
	return int2(col, row);
}

/**
 * @brief Returns the TexCoords of a FarTexture in the TextureAtlas.
 */
float2 CFarTextureHandler::GetTextureCoords(const int farTextureNum, const int orientation) const
{
	const int2 texIndex = GetTextureCoordsInt(farTextureNum, orientation);

	float2 texCoors;
	texCoors.x = (float(iconSize.x) / texSize.x) * texIndex.x;
	texCoors.y = (float(iconSize.y) / texSize.y) * texIndex.y;

	return texCoors;
}


bool CFarTextureHandler::HaveFarIcon(const CSolidObject* obj) const
{
	const int  teamID = obj->team;
	const int modelID = obj->model->id;

	return ((iconCache.size() > teamID) && (iconCache[teamID].size() > modelID) && (iconCache[teamID][modelID].farTexNum != 0));
}


/**
 * @brief Really create the far texture for the given model.
 */
void CFarTextureHandler::CreateFarTexture(const CSolidObject* obj)
{
	const S3DModel* model = obj->model;

	// make space in the std::vectors
	if (obj->team >= (int)iconCache.size())
		iconCache.resize(obj->team + 1);

	if (model->id >= (int)iconCache[obj->team].size())
		iconCache[obj->team].resize(model->id + 1, {0});

	assert(iconCache[obj->team][model->id].farTexNum == 0);

	// enough free space in the atlas?
	if (!CheckResizeAtlas())
		return;

	fbo.Bind();
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, texSize.x, texSize.y);
	fbo.CheckStatus("FARTEXTURE");

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_BLEND);
	glFrontFace(GL_CW);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glFogi(GL_FOG_MODE,   GL_LINEAR);
	glFogf(GL_FOG_START,  0.0f);
	glFogf(GL_FOG_END,    1e6);
	glFogf(GL_FOG_DENSITY, 1.0f);

	// NOTE:
	//   the icons are RTT'ed using a snapshot of the
	//   current state (advModelShading, sunDir, etc)
	//   and will not track later state-changes
	unitDrawer->SetupOpaqueDrawing(false);
	unitDrawer->PushModelRenderState(model);
	unitDrawer->SetTeamColour(obj->team);

	// can pick any perspective-type
	CCamera iconCam(CCamera::CAMTYPE_PLAYER);
	CMatrix44f viewMat;

	// twice the radius is not quite far away enough for some models
	viewMat.Translate(float3(0.0f, 0.0f, -obj->GetDrawRadius() * (2.0f + 1.0f)));
	viewMat.Scale(float3(-1.0f, 1.0f, 1.0f));
	viewMat.RotateX(-60.0f * (PI / 180.0f));

	// overwrite the matrices set by SetupOpaqueDrawing
	//
	// RTT with a 60-degree top-down view and 1:1 AR perspective
	// model shaders expect view-matrix on the PROJECTION stack!
	iconCam.UpdateMatrices(1, 1, 1.0f);
	iconCam.SetProjMatrix(iconCam.GetProjectionMatrix() * viewMat);
	iconCam.SetViewMatrix(viewMat.LoadIdentity());
	iconCam.LoadMatrices();

	for (int orient = 0; orient < NUM_ICON_ORIENTATIONS; ++orient) {
		// setup viewport
		const int2 pos = GetTextureCoordsInt(usedFarTextures, orient);

		glViewport(pos.x * iconSize.x, pos.y * iconSize.y, iconSize.x, iconSize.y);
		glClear(GL_DEPTH_BUFFER_BIT);

		glPushMatrix();
		// draw (static-pose) model
		model->DrawStatic();
		glPopMatrix();

		// rotate for the next orientation
		glRotatef(-360.0f / NUM_ICON_ORIENTATIONS, 0.0f, 1.0f, 0.0f);
	}

	unitDrawer->PopModelRenderState(model);
	unitDrawer->ResetOpaqueDrawing(false);

	// glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glPopAttrib();

	fbo.Detach(GL_DEPTH_ATTACHMENT_EXT);
	fbo.Unbind();

	// cache object's current radius s.t. quad is always drawn with fixed size
	iconCache[obj->team][model->id].farTexNum = ++usedFarTextures;
	iconCache[obj->team][model->id].texScales = {obj->GetDrawRadius(), obj->GetDrawRadius()};
	iconCache[obj->team][model->id].texOffset = UpVector * obj->GetDrawRadius() * 0.5f;
}


void CFarTextureHandler::DrawFarTexture(const CSolidObject* obj, CVertexArray* va)
{
	const CachedIcon& icon = iconCache[obj->team][obj->model->id];

	// not found in the atlas
	if (icon.farTexNum == 0)
		return;

	// indicates the orientation to draw
	const int USHRT_MAX_ = (1 << 16) - 1;
	const int orientStep = USHRT_MAX_ / NUM_ICON_ORIENTATIONS;

	int orient = GetHeadingFromVector(-camera->GetDir().x, -camera->GetDir().z) - obj->heading;
		orient += USHRT_MAX_;         // make it positive only
		orient += (orientStep >> 1);  // we want that frontdir is from -orientStep/2 upto orientStep/2
		orient %= USHRT_MAX_;         // we have an angle so it's periodical
		orient /= orientStep;         // get the final direction index

	const float2 objIconSize = {float(iconSize.x) / texSize.x, float(iconSize.y) / texSize.y};
	const float2 objTexCoors = GetTextureCoords(icon.farTexNum - 1, orient);

	// have to draw above ground, or quad will be clipped
	const float3 pos = obj->drawPos + icon.texOffset;
	const float3 upv = camera->GetUp()    * icon.texScales.y;
	const float3 rgv = camera->GetRight() * icon.texScales.x;

	va->AddVertexQT(pos - upv + rgv, objTexCoors.x,                 objTexCoors.y                );
	va->AddVertexQT(pos + upv + rgv, objTexCoors.x,                 objTexCoors.y + objIconSize.y);
	va->AddVertexQT(pos + upv - rgv, objTexCoors.x + objIconSize.x, objTexCoors.y + objIconSize.y);
	va->AddVertexQT(pos - upv - rgv, objTexCoors.x + objIconSize.x, objTexCoors.y                );
}


void CFarTextureHandler::Queue(const CSolidObject* obj)
{
	drawQueue.push_back(obj);
}


void CFarTextureHandler::Draw()
{
	if (drawQueue.empty())
		return;

	if (!fbo.IsValid()) {
		drawQueue.clear();
		return;
	}

	// create new far-icons
	for (const CSolidObject* obj: drawQueue) {
		if (!HaveFarIcon(obj)) {
			CreateFarTexture(obj);
		}
	}

	// render current queued far icons on the screen
	{
		const float3 camNorm = ((camera->GetDir() * XZVector) - (UpVector * 0.1f)).ANormalize();

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, farTextureID);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glNormal3fv((const GLfloat*) &camNorm.x);

		sky->SetupFog();

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(drawQueue.size() * 4, 0, VA_SIZE_T);

		for (const CSolidObject* obj: drawQueue) {
			DrawFarTexture(obj, va);
		}

		va->DrawArrayT(GL_QUADS);
		glDisable(GL_ALPHA_TEST);
	}

	drawQueue.clear();
}



bool CFarTextureHandler::CheckResizeAtlas()
{
	const unsigned int maxSprites = ((texSize.x / iconSize.x) * (texSize.y / iconSize.y) / NUM_ICON_ORIENTATIONS) - 1;

	if (usedFarTextures + 1 <= maxSprites)
		return true;

	const int oldTexSizeY = texSize.y;

	if (globalRendering->supportNPOTs) {
		// make space for minimum 4 additional icons
		texSize.y += std::max(iconSize.y,  4 * NUM_ICON_ORIENTATIONS * iconSize.x * iconSize.y / texSize.x);
	} else {
		texSize.y <<= 1;
	}

	if (texSize.y > globalRendering->maxTextureSize) {
		LOG_L(L_DEBUG, "Out of farTextures");
		texSize.y = oldTexSizeY;
		return false;
	}

	std::vector<unsigned char> oldPixels(texSize.x * texSize.y * 4);
	glBindTexture(GL_TEXTURE_2D, farTextureID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &oldPixels[0]); //TODO use the FBO?
	memset(&oldPixels[0] + texSize.x*oldTexSizeY*4, 0, texSize.x*(texSize.y - oldTexSizeY)*4);

	GLuint newFarTextureID;
	glGenTextures(1, &newFarTextureID);
	glBindTexture(GL_TEXTURE_2D, newFarTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texSize.x, texSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &oldPixels[0]);

	fbo.Bind();
	fbo.DetachAll();

	glDeleteTextures(1, &farTextureID);
	farTextureID = newFarTextureID;

	fbo.AttachTexture(farTextureID);
	fbo.CheckStatus("FARTEXTURE");
	fbo.Unbind();

	return true;
}

