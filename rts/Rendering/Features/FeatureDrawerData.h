#pragma once

#include "System/float3.h"
#include "Rendering/Common/ModelDrawerData.h"

class CFeature;
class CCamera;

class CFeatureDrawerData : public CFeatureDrawerDataBase {
public:
	// CEventClient interface
	bool WantsEvent(const std::string & eventName) {
		return
			eventName == "RenderFeatureCreated"   ||
			eventName == "RenderFeatureDestroyed";
	}
	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureDestroyed(const CFeature* feature);
public:
	CFeatureDrawerData(bool& mtModelDrawer_);
	virtual ~CFeatureDrawerData();
public:
	void ConfigNotify(const std::string& key, const std::string& value);
public:
	void Update() override;
	bool IsAlpha(const CFeature* co) const override;
protected:
	void UpdateObjectDrawFlags(CSolidObject* o) const override;
private:
	static void UpdateDrawPos(CFeature* f);
public:
	float featureDrawDistance;
	float featureFadeDistance;
	float featureFadeDistanceSq;
	float featureDrawDistanceSq;
};