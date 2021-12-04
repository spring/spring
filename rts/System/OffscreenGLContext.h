/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OFFSCREENGLCONTEXT_H
#define _OFFSCREENGLCONTEXT_H

#include "Rendering/GL/myGL.h"

#include <functional>
#include "System/Threading/SpringThreading.h"

/**
 * @brief COffscreenGLThread
 * Runs a std::bind in an additional thread with an offscreen OpenGL context.
 * (Don't try render to the 'screen' a.k.a. default framebuffer in that thread, the results will be undetermistic)
 */
class COffscreenGLThread
{
public:
	COffscreenGLThread() = default;
	COffscreenGLThread(std::function<void()> f);
	~COffscreenGLThread() { join(); }
	COffscreenGLThread(const COffscreenGLThread& t) = delete;
	COffscreenGLThread(COffscreenGLThread&& t) { *this = std::move(t); }

	COffscreenGLThread& operator = (const COffscreenGLThread& t) = delete;
	COffscreenGLThread& operator = (COffscreenGLThread&& t) {
		thread = std::move(t.thread);
		return *this;
	};

	void join();
	bool joinable() const { return (thread.joinable()); }

private:
	void WrapFunc(std::function<void()> f);

	spring::thread thread;
};


#endif // _OFFSCREENGLCONTEXT_H
