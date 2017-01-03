/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Screenshot.h"

#include <vector>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/ThreadPool.h"

#undef CreateDirectory

CONFIG(int, ScreenshotCounter).defaultValue(0);

struct FunctionArgs
{
	std::vector<uint8_t> buf;
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
	args.buf.resize(args.x * args.y * 4);

	char buf[512] = {0};

	for (int a = configHandler->GetInt("ScreenshotCounter"); a <= 99999; ++a) {
		snprintf(&buf[0], sizeof(buf), "screenshots/screen%05d.%s", a, type.c_str());

		CFileHandler ifs(buf);

		if (!ifs.FileExists()) {
			configHandler->Set("ScreenshotCounter", a < 99999 ? a+1 : 0);
			args.filename = buf;
			break;
		}
	}

	glReadPixels(0, 0, args.x, args.y, GL_RGBA, GL_UNSIGNED_BYTE, &args.buf[0]);

	ThreadPool::Enqueue([](const FunctionArgs& args) {
		CBitmap bmp(&args.buf[0], args.x, args.y);
		bmp.ReverseYAxis();
		bmp.Save(args.filename);
		LOG("Saved: %s", args.filename.c_str());
	}, args);
}
