#ifndef MATRIX_UPLOADER_H
#define MATRIX_UPLOADER_H

#include <cstdint>
#include <vector>
#include <unordered_set>

#include "System/EventClient.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"

/*
struct MatrixInfo {
	uint32_t elemOffset;
};
*/
struct VBO;

class ProxyProjectileListener : public CEventClient {
public:
	static ProxyProjectileListener& GetInstance() {
		static ProxyProjectileListener instance;
		return instance;
	};
public:
	ProxyProjectileListener() : CEventClient("[ProxyProjectileListener]", 123455, false) { eventHandler.AddClient(this); }
	virtual ~ProxyProjectileListener() { eventHandler.RemoveClient(this); }
	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "RenderProjectileCreated") ||
			(eventName == "RenderProjectileDestroyed");
	}
	void RenderProjectileCreated(const CProjectile* projectile);
	void RenderProjectileDestroyed(const CProjectile* projectile);
};

class MatrixUploader {
public:
	static const bool enabled = false; ////////!!!!!!!!!!!!!!!!!!!!
	static MatrixUploader& GetInstance() {
		static MatrixUploader instance;
		return instance;
	};
	static bool Supported() {
		static bool supported = enabled && VBO::IsSupported(GL_UNIFORM_BUFFER) && VBO::IsSupported(GL_SHADER_STORAGE_BUFFER) && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
		return supported;
	}
public:
	void Init();
	void Kill();
	void UpdateAndBind();
public:
	friend class ProxyProjectileListener;
private:
	template<typename TObj>
	bool IsObjectVisible(const TObj* obj);
	template<typename TObj>
	bool IsInView(const TObj* obj);
	template<typename TObj>
	void GetVisibleObjects();
private:
	void KillVBO();
	void InitVBO(const uint32_t newElemCount);
	uint32_t GetMatrixElemCount();
private:
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 4;
	static constexpr uint32_t elemCount0 = 1u << 12;
	static constexpr uint32_t elemIncreaseBy = 1u << 9;
	static constexpr uint32_t elemSizingDownDiv = 4;
private:
	bool initialized = false;

	ProxyProjectileListener* proxyProjectileListener;

	std::vector<const CUnit*> visibleUnits;
	std::vector<const CFeature*> visibleFeatures;
	std::vector<const CProjectile*> visibleProjectiles;

	std::unordered_set<const CProjectile*> visibleProjectilesSet;

	std::vector<CMatrix44f> matrices;

	VBO* matrixSSBO;
};

#endif //MATRIX_UPLOADER_H