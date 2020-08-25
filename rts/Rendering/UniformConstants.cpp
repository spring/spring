#include "UniformConstants.h"

#include <cassert>

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


void UniformConstants::Init()
{
	if (!Supported()) {
		LOG_L(L_ERROR, "[UniformConstants::%s] Important OpenGL extensions are not supported by the system\n  GLEW_ARB_uniform_buffer_object = %d\n  GLEW_ARB_shading_language_420pack = %d", __func__, GLEW_ARB_uniform_buffer_object, GLEW_ARB_shading_language_420pack);
		return;
	}

	{
		GLint uniformBufferOffset = 0;

		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferOffset);

		umbBufferSize = ((sizeof(UniformMatricesBuffer) / uniformBufferOffset) + 1) * uniformBufferOffset;
		upbBufferSize = ((sizeof(UniformParamsBuffer) / uniformBufferOffset) + 1) * uniformBufferOffset;
	}

	for (VBO*& buf : umbBuffers) {
		buf = new VBO(GL_UNIFORM_BUFFER, PERSISTENT_STORAGE);
		buf->Bind(GL_UNIFORM_BUFFER);
		buf->New(umbBufferSize, GL_DYNAMIC_DRAW);
		buf->Unbind();
	}

	for (VBO*& buf : upbBuffers) {
		buf = new VBO(GL_UNIFORM_BUFFER, PERSISTENT_STORAGE);
		buf->Bind(GL_UNIFORM_BUFFER);
		buf->New(upbBufferSize, GL_DYNAMIC_DRAW);
		buf->Unbind();
	}
}

void UniformConstants::Kill()
{
	if (!Supported())
		return;

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIX_IDX, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_PARAMS_IDX, 0);

	for (VBO*& buf : umbBuffers) {
		buf->Release();
		spring::SafeDelete(buf);
	}

	for (VBO*& buf : upbBuffers) {
		buf->Release();
		spring::SafeDelete(buf);
	}
}

void UniformConstants::UpdateMatrices(UniformMatricesBuffer* updateBuffer)
{
	if (!Supported())
		return;

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

void UniformConstants::Update()
{
	auto buffCurIdx = globalRendering->drawFrame % BUFFERING;
	umbBuffer = umbBuffers[buffCurIdx];
	upbBuffer = upbBuffers[buffCurIdx];

	umbBuffer->Bind(GL_UNIFORM_BUFFER);

	if (umbBufferMap == nullptr)
		umbBufferMap = reinterpret_cast<UniformMatricesBuffer*>(umbBuffer->MapBuffer(0, umbBufferSize));

	assert(umbBufferMap != nullptr);
	UpdateMatrices(umbBufferMap);

	if (PERSISTENT_STORAGE == false) {
		umbBuffer->UnmapBuffer();
		umbBufferMap = nullptr;
	}

	umbBuffer->Unbind();

	upbBuffer->Bind(GL_UNIFORM_BUFFER);

	if (upbBufferMap == nullptr)
		upbBufferMap = reinterpret_cast<UniformParamsBuffer*>(upbBuffer->MapBuffer(0, upbBufferSize));

	assert(umbBufferMap != nullptr);
	UpdateParams(upbBufferMap);

	if (PERSISTENT_STORAGE == false) {
		upbBuffer->UnmapBuffer();
		upbBufferMap = nullptr;
	}

	upbBuffer->Unbind();
}

//lazy / implicit unbinding included
void UniformConstants::Bind()
{
	if (!Supported())
		return;

	assert(umbBuffer != nullptr && upbBuffer != nullptr);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIX_IDX, umbBuffer->GetId());
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_PARAMS_IDX, upbBuffer->GetId());
}