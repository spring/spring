/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __LOADSCREEN_H__
#define __LOADSCREEN_H__

#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "GameController.h"
/// #include "Rendering/GL/myGL.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/OffscreenGLContext.h"
#include "myTime.h"


class CLoadScreen : public CGameController
{
public:
	void SetLoadMessage(const std::string& text, bool replace_lastline = false);

	CLoadScreen(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	virtual ~CLoadScreen();

	/// Splitt off from the ctor, casue this already uses GetInstance().
	void Init();

public:
	static CLoadScreen* GetInstance() {
		assert(singleton);
		return singleton;
	}
	static void CreateInstance(const std::string& mapName, const std::string& modName, ILoadSaveHandler* saveFile);
	static void DeleteInstance();

	bool Draw();
	bool Update();

	void ResizeEvent();

	/// Called when a key is released by the user
	int KeyReleased(unsigned short k);
	/// Called when the key is pressed by the user (can be called several times due to key repeat)
	int KeyPressed(unsigned short k,bool isRepeat);

private:
	void RandomStartPicture(const std::string& sidePref);
	void LoadStartPicture(const std::string& picture);
	void UnloadStartPicture();

private:
	static CLoadScreen* singleton;

	std::string oldLoadMessages;
	std::string curLoadMessage;

	std::string mapName;
	std::string modName;
	ILoadSaveHandler* saveFile;

	boost::recursive_mutex mutex;
	boost::thread* netHeartbeatThread;
	COffscreenGLThread* gameLoadThread;
	bool mt_loading;

	unsigned int startupTexture;
	float aspectRatio;
	spring_time last_draw;
};


#define loadscreen CLoadScreen::GetInstance()


#endif // __LOADSCREEN_H__
