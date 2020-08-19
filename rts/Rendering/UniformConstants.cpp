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
		buffSize = ((sizeof(UniformConstantsBuffer) / uniformBufferOffset) + 1) * uniformBufferOffset;
	}

	for (auto& ucbBuffer : ucbBuffers) {
		ucbBuffer = new VBO(GL_UNIFORM_BUFFER, PERSISTENT_STORAGE);
		ucbBuffer->Bind(GL_UNIFORM_BUFFER);
		ucbBuffer->New(buffSize, GL_DYNAMIC_DRAW);
		ucbBuffer->Unbind();
	}
}

void UniformConstants::Kill()
{
	if (!Supported())
		return;

	for (auto& ucbBuffer : ucbBuffers) {
		ucbBuffer->Release();
		spring::SafeDelete(ucbBuffer);
	}
}

void UniformConstants::UpdateMatrices(UniformConstantsBuffer* updateBuffer)
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


void UniformConstants::UpdateParams(UniformConstantsBuffer* updateBuffer)
{
	updateBuffer->timeInfo = float4{(float)gs->frameNum, gs->frameNum / (1.0f * GAME_SPEED), (float)globalRendering->drawFrame, globalRendering->timeOffset}; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	updateBuffer->viewGemetry = float4{(float)globalRendering->viewSizeX, (float)globalRendering->viewSizeY, (float)globalRendering->viewPosX, (float)globalRendering->viewPosY}; //vsx, vsy, vpx, vpy
	updateBuffer->mapSize = float4{(float)mapDims.mapx, (float)mapDims.mapy, (float)mapDims.pwr2mapx, (float)mapDims.pwr2mapy} *(float)SQUARE_SIZE; //xz, xzPO2
}

void UniformConstants::Update()
{
	auto buffSlice = globalRendering->drawFrame % BUFFERING;
	ucbBuffer = ucbBuffers[buffSlice];

	ucbBuffer->Bind(GL_UNIFORM_BUFFER);
	auto updateBuffer = reinterpret_cast<UniformConstantsBuffer*>(ucbBuffer->MapBuffer(0, buffSize));
	assert(updateBuffer);

	UpdateMatrices(updateBuffer);
	UpdateParams(updateBuffer);

	ucbBuffer->UnmapBuffer();
	ucbBuffer->Unbind();
}

void UniformConstants::Bind(const int bindIndex)
{
	if (!Supported())
		return;

	assert(ucbBuffer && !bound);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindIndex, ucbBuffer->GetId());
}

void UniformConstants::Unbind(const int bindIndex)
{
	if (!Supported())
		return;

	assert(ucbBuffer && bound);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindIndex, 0);
}