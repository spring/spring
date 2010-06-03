/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Screenshot.h"

#include <vector>
#include <iomanip>
#include <boost/thread.hpp>

#include "FileSystem/FileSystem.h"
#include "FileSystem/FileHandler.h"
#include "ConfigHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "GlobalUnsynced.h"
#include "LogOutput.h"

#undef CreateDirectory

struct FunctionArgs
{
	boost::uint8_t* buf;
	std::string filename;
	int x;
	int y;
};

class SaverThread
{
public:
	SaverThread() : myThread(NULL), finished(false) {};
	~SaverThread()
	{
		if (myThread)
		{
			myThread->join();
			delete myThread;
		}
	};
	
	void AddTask(FunctionArgs arg)
	{
		{
			boost::mutex::scoped_lock mylock(myMutex);
			tasks.push_back(arg);
			Update();
		}
		
		if (!myThread)
		{
			finished = false;
			myThread = new boost::thread(boost::bind(&SaverThread::SaveStuff, this));
		}
	};
	
	void Update()
	{
		if (finished && myThread)
		{
			myThread->join();
			delete myThread;
			myThread = NULL;
			finished = false;
		}
	};
	
private:
	bool GetTask(FunctionArgs& args)
	{
		boost::mutex::scoped_lock mylock(myMutex);
		if (!tasks.empty())
		{
			args = tasks.front();
			tasks.pop_front();
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void SaveStuff()
	{
		FunctionArgs args;
		while (GetTask(args))
		{
			CBitmap b(args.buf, args.x, args.y);
			delete[] args.buf;
			b.ReverseYAxis();
			b.Save(args.filename);
			LogObject() << "Saved: " << args.filename;
		}
		finished = true;
	};
	
	boost::mutex myMutex;
	boost::thread* myThread;
	volatile bool finished;
	std::list<FunctionArgs> tasks;
};

SaverThread screenshotThread;

void TakeScreenshot(std::string type)
{
	if (type.empty())
		type = "png";

	if (filesystem.CreateDirectory("screenshots"))
	{
		FunctionArgs args;
		args.x = globalRendering->dualScreenMode? globalRendering->viewSizeX << 1: globalRendering->viewSizeX;
		args.y = globalRendering->viewSizeY;

		if (args.x % 4)
			args.x += (4 - args.x % 4);
		
		for (int a = configHandler->Get("ScreenshotCounter", 0); a <= 99999; ++a)
		{
			std::ostringstream fname;
			fname << "screenshots/screen" << std::setfill('0') << std::setw(5) << a << '.' << type;
			CFileHandler ifs(fname.str());
			if (!ifs.FileExists())
			{
				configHandler->Set("ScreenshotCounter", a < 99999 ? a+1 : 0);
				args.filename = fname.str();
				break;
			}
		}

		args.buf = new boost::uint8_t[args.x * args.y * 4];
		glReadPixels(0, 0, args.x, args.y, GL_RGBA, GL_UNSIGNED_BYTE, args.buf);
		screenshotThread.AddTask(args);
	}
}
