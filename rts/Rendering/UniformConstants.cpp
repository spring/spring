#include "UniformConstants.h"

#include <cassert>
#include <stdint.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
#include "SDL2/SDL_mouse.h"

void UniformConstants::InitVBO(VBO*& vbo, const int vboSingleSize)
{
	vbo = new VBO{GL_UNIFORM_BUFFER, WantPersistentMapping()};
	vbo->Bind(GL_UNIFORM_BUFFER);
	vbo->New(BUFFERING * vboSingleSize, GL_STREAM_DRAW); //allocate BUFFERING times the buffer size for non-blocking updates
	vbo->Unbind();
}


void UniformConstants::Init()
{
	if (initialized) //don't need to reinit on resolution changes
		return;

	if (!Supported()) {
		LOG_L(L_ERROR, "[UniformConstants::%s] Important OpenGL extensions are not supported by the system\n  GLEW_ARB_uniform_buffer_object = %d\n  GLEW_ARB_shading_language_420pack = %d", __func__, GLEW_ARB_uniform_buffer_object, GLEW_ARB_shading_language_420pack);
		return;
	}

	umbBufferSize = VBO::GetAlignedSize(GL_UNIFORM_BUFFER, sizeof(UniformMatricesBuffer));
	upbBufferSize = VBO::GetAlignedSize(GL_UNIFORM_BUFFER, sizeof(UniformParamsBuffer  ));

	InitVBO(umbVBO, umbBufferSize);
	InitVBO(upbVBO, upbBufferSize);

	initialized = true;
}

void UniformConstants::Kill()
{
	if (!Supported() || !initialized)
		return;

	//unbind the whole ring buffer range
	umbVBO->UnbindBufferRange(GL_UNIFORM_BUFFER, UBO_MATRIX_IDX, 0, BUFFERING * umbBufferSize);
	upbVBO->UnbindBufferRange(GL_UNIFORM_BUFFER, UBO_PARAMS_IDX, 0, BUFFERING * upbBufferSize);

	spring::SafeDelete(umbVBO);
	spring::SafeDelete(upbVBO);

	initialized = false;
}

intptr_t UniformConstants::GetBufferOffset(const int vboSingleSize)
{
	unsigned int buffCurIdx = globalRendering->drawFrame % BUFFERING;
	return (intptr_t)(buffCurIdx * vboSingleSize);
}

void UniformConstants::UpdateMatricesImpl(UniformMatricesBuffer* updateBuffer)
{
	updateBuffer->screenView = globalRendering->screenViewMatrix;
	updateBuffer->screenProj = globalRendering->screenProjMatrix;
	updateBuffer->screenViewProj = updateBuffer->screenProj * updateBuffer->screenView;

	const auto camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	updateBuffer->cameraView = camPlayer->GetViewMatrix();
	updateBuffer->cameraProj = camPlayer->GetProjectionMatrix();
	updateBuffer->cameraViewProj = camPlayer->GetViewProjectionMatrix();

	// pretty useless as billboarding should be applied to modelView matrix and not viewMatrix
	// much easier way is to assign identity matrix to top-left 3x3 submatrix in the shader
	updateBuffer->cameraBillboardView = updateBuffer->cameraView * camPlayer->GetBillBoardMatrix(); //GetBillBoardMatrix() is supposed to be multiplied by the view Matrix

	updateBuffer->cameraViewInv = camPlayer->GetViewMatrixInverse();
	updateBuffer->cameraProjInv = camPlayer->GetProjectionMatrixInverse();
	updateBuffer->cameraViewProjInv = camPlayer->GetViewProjectionMatrixInverse();

	updateBuffer->shadowView = shadowHandler.GetShadowViewMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING);
	updateBuffer->shadowProj = shadowHandler.GetShadowProjMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING);
	updateBuffer->shadowViewProj = updateBuffer->shadowProj * updateBuffer->shadowView;

	updateBuffer->orthoProj01 = CMatrix44f::ClipOrthoProj01();

	updateBuffer->mmDrawView = minimap->GetViewMat(0);
	updateBuffer->mmDrawIMMView = minimap->GetViewMat(1);
	updateBuffer->mmDrawDimView = minimap->GetViewMat(2);

	updateBuffer->mmDrawProj = minimap->GetProjMat(0);
	updateBuffer->mmDrawIMMProj = minimap->GetProjMat(1);
	updateBuffer->mmDrawDimProj = minimap->GetProjMat(2);

	updateBuffer->mmDrawViewProj = updateBuffer->mmDrawProj * updateBuffer->mmDrawView;
	updateBuffer->mmDrawIMMViewProj = updateBuffer->mmDrawIMMProj * updateBuffer->mmDrawIMMView;
	updateBuffer->mmDrawDimViewProj = updateBuffer->mmDrawDimProj * updateBuffer->mmDrawDimView;
}


void UniformConstants::UpdateParamsImpl(UniformParamsBuffer* updateBuffer)
{
	updateBuffer->rndVec3 = guRNG.NextVector();
	//TODO add something else
	updateBuffer->renderCaps =
		globalRendering->supportClipSpaceControl << 0;

	updateBuffer->timeInfo = float4{(float)gs->frameNum, gs->frameNum / (1.0f * GAME_SPEED), (float)globalRendering->drawFrame, globalRendering->timeOffset}; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	updateBuffer->viewGeometry = float4{(float)globalRendering->viewSizeX, (float)globalRendering->viewSizeY, (float)globalRendering->viewPosX, (float)globalRendering->viewPosY}; //vsx, vsy, vpx, vpy
	updateBuffer->mapSize = float4{(float)mapDims.mapx, (float)mapDims.mapy, (float)mapDims.pwr2mapx, (float)mapDims.pwr2mapy} *(float)SQUARE_SIZE; //xz, xzPO2
	updateBuffer->mapHeight = float4{readMap->GetCurrMinHeight(), readMap->GetCurrMaxHeight(), readMap->GetInitMinHeight(), readMap->GetInitMaxHeight()};

	float4 fogColor = (sky != nullptr) ? float4{sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 1.0f} : float4{0.7f, 0.7f, 0.8f, 1.0f};
	updateBuffer->fogColor = fogColor;

	const auto camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	float4 fogParams = (sky != nullptr) ? float4{sky->fogStart * camPlayer->GetFarPlaneDist(), sky->fogEnd * camPlayer->GetFarPlaneDist(), 0.0f, 0.0f} : float4{0.1f * CGlobalRendering::MAX_VIEW_RANGE, 1.0f * CGlobalRendering::MAX_VIEW_RANGE, 0.0f, 0.0f};
	fogParams.w = 1.0f / (fogParams.y - fogParams.x);
	updateBuffer->fogParams = fogParams;

	updateBuffer->sunDir = (sky != nullptr) ? sky->GetLight()->GetLightDir() : float4(/*map default*/ 0.0f, 0.447214f, 0.894427f, 1.0f);

	updateBuffer->sunAmbientModel = sunLighting->modelAmbientColor;
	updateBuffer->sunAmbientMap = sunLighting->groundAmbientColor;

	updateBuffer->sunDiffuseModel = sunLighting->modelDiffuseColor;
	updateBuffer->sunDiffuseMap = sunLighting->groundDiffuseColor;

	updateBuffer->sunSpecularModel = sunLighting->modelSpecularColor;
	updateBuffer->sunSpecularMap = sunLighting->groundSpecularColor;

	updateBuffer->shadowDensity = float4{ sunLighting->groundShadowDensity, sunLighting->modelShadowDensity, 0.0, 0.0 };

	updateBuffer->windInfo = float4{ envResHandler.GetCurrentWindVec(), envResHandler.GetCurrentWindStrength() };

	updateBuffer->mouseScreenPos = float2{
		static_cast<float>(mouse->lastx - globalRendering->viewPosX),
		static_cast<float>(globalRendering->viewSizeY - mouse->lasty - 1)
	};

	updateBuffer->mouseStatus = (
		mouse->buttons[SDL_BUTTON_LEFT  ].pressed << 0 |
		mouse->buttons[SDL_BUTTON_MIDDLE].pressed << 1 |
		mouse->buttons[SDL_BUTTON_RIGHT ].pressed << 2 |
		mouse->offscreen                          << 3 |
		mouse->mmbScroll                          << 4 |
		mouse->locked                             << 5
	);
	updateBuffer->mouseUnused = 0u;

	{
		const int wx = mouse->lastx;
		const int wy = mouse->lasty - globalRendering->viewPosY;

		const CUnit* unit = nullptr;
		const CFeature* feature = nullptr;

		const float rawRange = camPlayer->GetFarPlaneDist() * 1.4f;
		const float badRange = rawRange - 300.0f;

		const float3 camPos = camPlayer->GetPos();
		const float3 pxlDir = camPlayer->CalcPixelDir(wx, wy);

		// trace for player's allyteam
		const float traceDist = TraceRay::GuiTraceRay(camPos, pxlDir, rawRange, nullptr, unit, feature, true, false, true);

		const float3 tracePos = camPos + (pxlDir * traceDist);

		if (unit)
			updateBuffer->mouseWorldPos = float4{ unit->drawPos, 1.0f };
		else if (feature)
			updateBuffer->mouseWorldPos = float4{ feature->drawPos, 1.0f };
		else
			updateBuffer->mouseWorldPos = float4{ tracePos, 1.0f };

		if ((traceDist < 0.0f || traceDist > badRange) && unit == nullptr && feature == nullptr) {
			updateBuffer->mouseWorldPos.w = 0.0f;
		}
	}

	for (int teamID = 0; teamID < teamHandler.ActiveTeams(); ++teamID) {
		const CTeam* team = teamHandler.Team(teamID);
		if (team == nullptr || !teamHandler.IsActiveTeam(teamID))
			continue;

		updateBuffer->teamColor[teamID] = float4{team->color[0] / 255.0f, team->color[1] / 255.0f, team->color[2] / 255.0f, team->color[3] / 255.0f};
	}
}

template<typename TBuffType, typename TUpdateFunc>
void UniformConstants::UpdateMapStandard(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize)
{
	vbo->Bind(GL_UNIFORM_BUFFER);
	buffMap = reinterpret_cast<TBuffType*>(vbo->MapBuffer(GetBufferOffset(vboSingleSize), vboSingleSize));
	assert(buffMap != nullptr);

	updateFunc(buffMap);

	vbo->UnmapBuffer();
	vbo->Unbind();
}

template<typename TBuffType, typename TUpdateFunc>
void UniformConstants::UpdateMapPersistent(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize)
{
	TBuffType* thisFrameBuffMap = nullptr;

	if (buffMap == nullptr) { //
		vbo->Bind(GL_UNIFORM_BUFFER); //bind only first time
		buffMap = reinterpret_cast<TBuffType*>(vbo->MapBuffer(0, BUFFERING * vboSingleSize)); //map first time and forever
		assert(buffMap != nullptr);
	}

	thisFrameBuffMap = reinterpret_cast<TBuffType*>((intptr_t)(buffMap) + GetBufferOffset(vboSingleSize)); //choose the current part of the buffer

	updateFunc(thisFrameBuffMap);

	//no need to unmap

	if (vbo->bound) //unbind only first time
		vbo->Unbind();
}


template<typename TBuffType, typename TUpdateFunc>
void UniformConstants::UpdateMap(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize)
{
	if (WantPersistentMapping())
		UpdateMapPersistent(vbo, buffMap, updateFunc, vboSingleSize);
	else
		UpdateMapStandard  (vbo, buffMap, updateFunc, vboSingleSize);
}

void UniformConstants::UpdateMatrices()
{
	if (!Supported())
		return;

	UpdateMap(umbVBO, umbBufferMap, UniformConstants::UpdateMatricesImpl, umbBufferSize);
}

void UniformConstants::UpdateParams()
{
	if (!Supported())
		return;

	UpdateMap(upbVBO, upbBufferMap, UniformConstants::UpdateParamsImpl, upbBufferSize);
}

//lazy / implicit unbinding included
void UniformConstants::Bind()
{
	if (!Supported())
		return;

	assert(umbVBO != nullptr && upbVBO != nullptr);

	umbVBO->BindBufferRange(GL_UNIFORM_BUFFER, UBO_MATRIX_IDX, GetBufferOffset(umbBufferSize), umbBufferSize);
	upbVBO->BindBufferRange(GL_UNIFORM_BUFFER, UBO_PARAMS_IDX, GetBufferOffset(upbBufferSize), upbBufferSize);
}

bool UniformConstants::WantPersistentMapping()
{
	static bool wantPersistentMapping = globalRendering->supportPersistentMapping & false; //persistent mapping needs fences, tripple buffering alone is not enough :(
	return wantPersistentMapping;
}
