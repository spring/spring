/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Screenshot.h"

#include <vector>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/ThreadPool.h"

#undef CreateDirectory

CONFIG(int, ScreenshotCounter).defaultValue(0);

struct FunctionArgs
{
	std::vector<uint8_t> pixelbuf;
	std::string filename;
	int x;
	int y;
};

void TakeScreenshot(std::string type)
{
	if (type.empty())
		type = "jpg";

	if (!FileSystem::CreateDirectory("screenshots"))
		return;

	FunctionArgs args;
	args.x  = globalRendering->dualScreenMode? globalRendering->viewSizeX << 1: globalRendering->viewSizeX;
	args.y  = globalRendering->viewSizeY;
	args.x += ((4 - (args.x % 4)) * int((args.x % 4) != 0));

	const int shotCounter = configHandler->GetInt("ScreenshotCounter");

	// note: we no longer increment the counter until a "file not found" occurs
	// since that stalls the thread and might run concurrently with an IL write
	args.filename.assign("screenshots/screen" + IntToString(shotCounter, "%05d") + "." + type);
	args.pixelbuf.resize(args.x * args.y * 4);

	configHandler->Set("ScreenshotCounter", shotCounter + 1);

	glReadPixels(0, 0, args.x, args.y, GL_RGBA, GL_UNSIGNED_BYTE, &args.pixelbuf[0]);

	ThreadPool::Enqueue([](const FunctionArgs& args) {
		CBitmap bmp(&args.pixelbuf[0], args.x, args.y);
		bmp.ReverseYAxis();
		bmp.Save(args.filename, true, true);
	}, args);
}
