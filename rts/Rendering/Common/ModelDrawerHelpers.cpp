#include "ModelDrawerHelpers.h"
#include "ModelDrawer.h"
#include "System/float3.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Team.h"
#include "Sim/Objects/SolidObject.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Env/CubeMapHandler.h"

bool CModelDrawerHelper::ObjectVisibleReflection(const float3& objPos, const float3& camPos, float maxRadius)
{
	if (objPos.y < 0.0f)
		return (CGround::GetApproximateHeight(objPos.x, objPos.z, false) <= maxRadius);

	const float dif = objPos.y - camPos.y;

	float3 zeroPos;
	zeroPos += (camPos * ( objPos.y / dif));
	zeroPos += (objPos * (-camPos.y / dif));

	return (CGround::GetApproximateHeight(zeroPos.x, zeroPos.z, false) <= maxRadius);
}

void CModelDrawerHelper::EnableTexturesCommon()
{
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE2, true);

	if (CModelDrawerConcept::UseAdvShading()) {
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetEnvReflectionTextureID());

		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetSpecularTextureID());
	}

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
}

void CModelDrawerHelper::DisableTexturesCommon()
{
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.ResetShadowTexSampler(GL_TEXTURE2, true);

	if (CModelDrawerConcept::UseAdvShading()) {
		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_CUBE_MAP);

		glActiveTexture(GL_TEXTURE4);
		glDisable(GL_TEXTURE_CUBE_MAP);
	}

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}

void CModelDrawerHelper::PushTransform(const CCamera* cam)
{
	// set model-drawing transform; view is combined with projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMultMatrixf(cam->GetViewMatrix());
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void CModelDrawerHelper::PopTransform()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

float4 CModelDrawerHelper::GetTeamColor(int team, float alpha)
{
	assert(teamHandler.IsValidTeam(team));

	const   CTeam* t = teamHandler.Team(team);
	const uint8_t* c = t->color;

	return float4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, alpha);
}

void CModelDrawerHelper::DIDResetPrevProjection(bool toScreen)
{
	if (!toScreen)
		return;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPushMatrix();
}

void CModelDrawerHelper::DIDResetPrevModelView()
{
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPushMatrix();
}

bool CModelDrawerHelper::DIDCheckMatrixMode(int wantedMode)
{
#if 1
	int matrixMode = 0;
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
	return (matrixMode == wantedMode);
#else
	return true;
#endif
}


void CModelDrawerHelper::BindModelTypeTexture(int mdlType, int texType)
{
	const auto texMat = textureHandlerS3O.GetTexture(texType);

	if (shadowHandler.InShadowPass())
		modelDrawerHelpers[mdlType]->BindShadowTex(texMat);
	else
		modelDrawerHelpers[mdlType]->BindOpaqueTex(texMat);
}

void CModelDrawerHelper::PushModelRenderState(int mdlType)
{
	assert(CModelDrawerHelper::modelDrawerHelpers[mdlType]);
	modelDrawerHelpers[mdlType]->PushRenderState();
}

void CModelDrawerHelper::PushModelRenderState(const S3DModel* m)
{
	PushModelRenderState(m->type);
	BindModelTypeTexture(m->type, m->textureType);
}

void CModelDrawerHelper::PushModelRenderState(const CSolidObject* o) { PushModelRenderState(o->model); }

void CModelDrawerHelper::PopModelRenderState(int mdlType)
{
	assert(modelDrawerHelpers[mdlType]);
	modelDrawerHelpers[mdlType]->PopRenderState();
}

void CModelDrawerHelper::PopModelRenderState(const S3DModel* m) { PopModelRenderState(m->type); }
void CModelDrawerHelper::PopModelRenderState(const CSolidObject* o) { PopModelRenderState(o->model); }

///////////////////////////////////////////////////////////////////////////

const std::array<const CModelDrawerHelper*, MODELTYPE_CNT> CModelDrawerHelper::modelDrawerHelpers = {
	CModelDrawerHelper::GetInstance<CModelDrawerHelper3DO>(),
	CModelDrawerHelper::GetInstance<CModelDrawerHelperS3O>(),
	CModelDrawerHelper::GetInstance<CModelDrawerHelperASS>(),
};

///////////////////////////////////////////////////////////////////////////

void CModelDrawerHelper3DO::PushRenderState() const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureHandler3DO.GetAtlasTex2ID());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureHandler3DO.GetAtlasTex1ID());

	glDisable(GL_CULL_FACE);
}

void CModelDrawerHelper3DO::PopRenderState() const
{
	glEnable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
}

void CModelDrawerHelper3DO::BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureHandler3DO.GetAtlasTex2ID());
}

void CModelDrawerHelper3DO::UnbindShadowTex() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}

///////////////////////////////////////////////////////////////////////////

void CModelDrawerHelperS3O::BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureMat->tex1);
}

void CModelDrawerHelperS3O::UnbindOpaqueTex() const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CModelDrawerHelperS3O::BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
}

void CModelDrawerHelperS3O::UnbindShadowTex() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}

///////////////////////////////////////////////////////////////////////////

void CModelDrawerHelperASS::BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureMat->tex1);
}

void CModelDrawerHelperASS::UnbindOpaqueTex() const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CModelDrawerHelperASS::BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
}

void CModelDrawerHelperASS::UnbindShadowTex() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}