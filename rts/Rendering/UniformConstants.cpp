#include "UniformConstants.h"

#include <cassert>
#include <stdint.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"

void UniformConstants::InitVBO(VBO*& vbo, const int vboSingleSize)
{
	vbo = new VBO{GL_UNIFORM_BUFFER, globalRendering->supportPersistentMapping};
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
	updateBuffer->screenView = *globalRendering->screenViewMatrix;
	updateBuffer->screenProj = *globalRendering->screenProjMatrix;
	updateBuffer->screenViewProj = updateBuffer->screenProj * updateBuffer->screenView;

	const auto camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	updateBuffer->cameraView = camPlayer->GetViewMatrix();
	updateBuffer->cameraProj = camPlayer->GetProjectionMatrix();
	updateBuffer->cameraViewProj = camPlayer->GetViewProjectionMatrix();
	updateBuffer->cameraBillboard = updateBuffer->cameraView * camPlayer->GetBillBoardMatrix(); //GetBillBoardMatrix() is supposed to be multiplied by the view Matrix

	updateBuffer->cameraViewInv = camPlayer->GetViewMatrixInverse();
	updateBuffer->cameraProjInv = camPlayer->GetProjectionMatrixInverse();
	updateBuffer->cameraViewProjInv = camPlayer->GetViewProjectionMatrixInverse();

	updateBuffer->shadowView = shadowHandler.GetShadowViewMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING);
	updateBuffer->shadowProj = shadowHandler.GetShadowProjMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING);
	updateBuffer->shadowViewProj = updateBuffer->shadowProj * updateBuffer->shadowView;

	updateBuffer->orthoProj01 = CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f);
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

	float4 forColor = (sky != nullptr) ? float4{sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 1.0f} : float4{0.7f, 0.7f, 0.8f, 1.0f};
	updateBuffer->fogColor = forColor;

	const auto camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	float4 fogParams = (sky != nullptr) ? float4{sky->fogStart * camPlayer->GetFarPlaneDist(), sky->fogEnd * camPlayer->GetFarPlaneDist(), 0.0f, 0.0f} : float4{0.1f * CGlobalRendering::MAX_VIEW_RANGE, 1.0f * CGlobalRendering::MAX_VIEW_RANGE, 0.0f, 0.0f};
	fogParams.w = 1.0f / (fogParams.y - fogParams.x);
	updateBuffer->fogParams = fogParams;

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
	if (globalRendering->supportPersistentMapping)
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
