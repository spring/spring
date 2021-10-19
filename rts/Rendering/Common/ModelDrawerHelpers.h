#pragma once

#include "System/float4.h"
//can't fwd-declare nested class
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/3DModel.h"


class float3;
class CCamera;

class CModelDrawerHelper {
public:
	// Render States Push/Pop
	static void BindModelTypeTexture(int mdlType, int texType);

	static void PushModelRenderState(int mdlType);
	static void PushModelRenderState(const S3DModel* m);
	static void PushModelRenderState(const CSolidObject* o);

	static void PopModelRenderState(int mdlType);
	static void PopModelRenderState(const S3DModel* m);
	static void PopModelRenderState(const CSolidObject* o);
public:
	virtual void PushRenderState() const = 0;
	virtual void PopRenderState() const = 0;

	virtual void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const = 0;
	virtual void UnbindOpaqueTex() const = 0;
	virtual void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const = 0;
	virtual void UnbindShadowTex()  const = 0;
public:
	// Auxilary
	static bool ObjectVisibleReflection(const float3& objPos, const float3& camPos, float maxRadius);

	static void EnableTexturesCommon();
	static void DisableTexturesCommon();

	static void PushTransform(const CCamera* cam);
	static void PopTransform();

	static float4 GetTeamColor(int team, float alpha);

	static void DIDResetPrevProjection(bool toScreen);
	static void DIDResetPrevModelView();
	static bool DIDCheckMatrixMode(int wantedMode);
public:
	template<typename T>
	static const CModelDrawerHelper* GetInstance() {
		static const T instance;
		return &instance;
	}
public:
	static const std::array<const CModelDrawerHelper*, MODELTYPE_CNT> modelDrawerHelpers;
};

class CModelDrawerHelper3DO : public CModelDrawerHelper {
public:
	// Inherited via CModelDrawerHelper
	void PushRenderState() const override;
	void PopRenderState() const override;
	void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {/*handled in PushRenderState()*/ }
	void UnbindOpaqueTex() const override {/*handled in PushRenderState()*/ }
	void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override;
	void UnbindShadowTex() const override;
};

class CModelDrawerHelperS3O : public CModelDrawerHelper {
public:
	// Inherited via CModelDrawerHelper
	void PushRenderState() const override {/* no need for primitve restart*/ };
	void PopRenderState() const override {/* no need for primitve restart*/ };
	void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override;
	void UnbindOpaqueTex() const override;
	void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override;
	void UnbindShadowTex() const override;
};

class CModelDrawerHelperASS : public CModelDrawerHelper {
public:
	// Inherited via CModelDrawerHelper
	void PushRenderState() const override { /*no-op*/ };
	void PopRenderState() const override { /*no-op*/ };
	void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override;
	void UnbindOpaqueTex() const override;
	void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override;
	void UnbindShadowTex() const override;
};