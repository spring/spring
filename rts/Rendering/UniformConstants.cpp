#include "UniformConstants.h"

//#include <cassert>
#include <stdint.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"

#define MYASSERT
#ifdef MYASSERT
	#define ASSERT(x) do { if( !(x) ) { LOG_L(L_ERROR, "[UniformConstants::%s] assertion failure. Line %d", __func__, __LINE__); /*abort();*/ } } while(0)
#else
	#define ASSERT(x) do { } while(0)
#endif

#define DRAWFRAME_LIMIT 32

void UniformConstants::InitVBO(VBO*& vbo, const int vboSingleSize)
{
	vbo = new VBO{GL_UNIFORM_BUFFER, globalRendering->supportPersistentMapping};
	vbo->Bind(GL_UNIFORM_BUFFER);
	vbo->New(BUFFERING * vboSingleSize, GL_DYNAMIC_DRAW); //allocate BUFFERING times the buffer size for non-blocking updates
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

	umbBufferSize = RoundBuffSizeUp<UniformMatricesBuffer>();
	upbBufferSize = RoundBuffSizeUp<UniformParamsBuffer  >();

	InitVBO(umbVBO, umbBufferSize);
	InitVBO(upbVBO, upbBufferSize);

	initialized = true;
}

void UniformConstants::Kill()
{
	if (!Supported() || !initialized)
		return;

	//unbind the whole ring buffer range
	glBindBufferRange(GL_UNIFORM_BUFFER, UBO_MATRIX_IDX, 0, 0, umbBufferSize * BUFFERING);
	glBindBufferRange(GL_UNIFORM_BUFFER, UBO_PARAMS_IDX, 0, 0, upbBufferSize * BUFFERING);

	spring::SafeDelete(umbVBO);
	spring::SafeDelete(upbVBO);

	initialized = false;
}

intptr_t UniformConstants::GetBufferOffset(const int vboSingleSize)
{
	unsigned int buffCurIdx = globalRendering->drawFrame % BUFFERING;
	return (intptr_t)(buffCurIdx * vboSingleSize);
}

void UniformConstants::UpdateMatrices(UniformMatricesBuffer* updateBuffer)
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
}


void UniformConstants::UpdateParams(UniformParamsBuffer* updateBuffer)
{
	updateBuffer->timeInfo = float4{(float)gs->frameNum, gs->frameNum / (1.0f * GAME_SPEED), (float)globalRendering->drawFrame, globalRendering->timeOffset}; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	updateBuffer->viewGeometry = float4{(float)globalRendering->viewSizeX, (float)globalRendering->viewSizeY, (float)globalRendering->viewPosX, (float)globalRendering->viewPosY}; //vsx, vsy, vpx, vpy
	updateBuffer->mapSize = float4{(float)mapDims.mapx, (float)mapDims.mapy, (float)mapDims.pwr2mapx, (float)mapDims.pwr2mapy} *(float)SQUARE_SIZE; //xz, xzPO2

	updateBuffer->rndVec3 = float4{guRNG.NextVector(), 0.0f};
}

template<typename TBuffType, typename TUpdateFunc>
void UniformConstants::UpdateMapStandard(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize)
{
	vbo->Bind(GL_UNIFORM_BUFFER);
	buffMap = reinterpret_cast<TBuffType*>(vbo->MapBuffer(GetBufferOffset(vboSingleSize), vboSingleSize));
	ASSERT(buffMap != nullptr);

	if (globalRendering->drawFrame <= DRAWFRAME_LIMIT)
		LOG_L(L_WARNING, "[%s] VBO=%p, buffMap=%p, vboSingleSize=%d", __func__ , static_cast<void*>(vbo), static_cast<void*>(buffMap), vboSingleSize);

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
		ASSERT(buffMap != nullptr);
	}

	thisFrameBuffMap = reinterpret_cast<TBuffType*>((intptr_t)(buffMap) + GetBufferOffset(vboSingleSize)); //choose the current part of the buffer

	if (globalRendering->drawFrame <= DRAWFRAME_LIMIT)
		LOG_L(L_WARNING, "[%s] VBO=%p, buffMap=%p, vboSingleSize=%d", __func__ , static_cast<void*>(vbo), static_cast<void*>(thisFrameBuffMap), vboSingleSize);

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

void UniformConstants::Update()
{
	if (!Supported())
		return;

	if (globalRendering->drawFrame <= DRAWFRAME_LIMIT)
		LOG_L(L_WARNING, "[%s] drawFrame=%u umbBufferMap=%p, upbBufferMap=%p, umbBufferSize=%d, upbBufferSize=%d", __func__ , globalRendering->drawFrame, static_cast<void*>(umbBufferMap), static_cast<void*>(upbBufferMap), umbBufferSize, upbBufferSize);

	UpdateMap(umbVBO, umbBufferMap, UniformConstants::UpdateMatrices, umbBufferSize);
	UpdateMap(upbVBO, upbBufferMap, UniformConstants::UpdateParams  , upbBufferSize);
}

//lazy / implicit unbinding included
void UniformConstants::Bind()
{
	if (!Supported())
		return;

	ASSERT(umbVBO != nullptr && upbVBO != nullptr);
	glBindBufferRange(GL_UNIFORM_BUFFER, UBO_MATRIX_IDX, umbVBO->GetId(), GetBufferOffset(umbBufferSize), umbBufferSize);
	glBindBufferRange(GL_UNIFORM_BUFFER, UBO_PARAMS_IDX, upbVBO->GetId(), GetBufferOffset(upbBufferSize), upbBufferSize);
}
