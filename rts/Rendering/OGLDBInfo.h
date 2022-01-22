#pragma once

#include <string>
#include <algorithm>
#include <future>

#include "System/type2.h"


class OGLDBInfo {
public:
	OGLDBInfo() = delete;
	OGLDBInfo(const std::string& glRenderer_, const std::string& myOS_);
	OGLDBInfo(const OGLDBInfo&) = delete;
	OGLDBInfo(OGLDBInfo&& rhs) noexcept { *this = std::move(rhs); }

	bool IsReady(uint32_t waitTimeMS = 0) const;
	bool GetResult(const int2& curCtx, std::string& msg);

	OGLDBInfo& operator= (const OGLDBInfo&) = delete;
	OGLDBInfo& operator= (OGLDBInfo&& rhs) {
		std::swap(glRenderer, rhs.glRenderer);
		std::swap(myOS, rhs.myOS);
		std::swap(fut, rhs.fut);
		std::swap(maxVer, rhs.maxVer);
		std::swap(id, rhs.id);
		std::swap(drv, rhs.drv);

		return *this;
	}
private:
	std::string glRenderer;
	std::string myOS;

	int2 maxVer;
	std::string id;
	std::string drv;

	std::future<bool> fut;
};