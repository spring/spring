/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if       defined AVI_CAPTURING

#include "AviVideoCapturing.h"


#include "System/LogOutput.h"

#include "System/GlobalUnsynced.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/AVIGenerator.h"
#include "lib/streflop/streflop_cond.h"
#include <SDL_mouse.h>
#include <SDL_events.h>

#include <string>


AviVideoCapturing::AviVideoCapturing()
	: capturing(false)
	, aviGenerator(NULL) {
}

AviVideoCapturing::~AviVideoCapturing() {
}


bool AviVideoCapturing::IsCapturingSupported() const {
	return true;
}

bool AviVideoCapturing::IsCapturing() const {
	return capturing;
}


void AviVideoCapturing::StopCapturing() {

	if (IsCapturing()) {
		capturing = false;
		SafeDelete(aviGenerator);
		//delete aviGenerator;
		//aviGenerator = NULL;
	}
}


void AviVideoCapturing::StartCapturing() {

	if (IsCapturing()) {
		logOutput.Print("Video capturing is already running.");
		return;
	}

	// Find a file to capture to
	std::string fileName;
	const size_t MAX_NUM_VIDEOS = 1000;
	size_t vi;
	for (vi = 0; vi < MAX_NUM_VIDEOS; ++vi) {
		fileName = std::string("video") + IntToString(vi) + ".avi";
		CFileHandler ifs(fileName);
		if (!ifs.FileExists()) {
			break;
		}
	}

	if (vi == MAX_NUM_VIDEOS) {
		logOutput.Print("Error: You have too many videos on disc already, please move, rename or delete some.");
		logOutput.Print("Error: Not creating video!");
	} else {
		capturing = true;
		const int videoSizeX = (globalRendering->viewSizeX / 4) * 4;
		const int videoSizeY = (globalRendering->viewSizeY / 4) * 4;
		aviGenerator = new CAVIGenerator(fileName, videoSizeX, videoSizeY, 30);

		const int savedCursorMode = SDL_ShowCursor(SDL_QUERY);
		SDL_ShowCursor(SDL_ENABLE);

		if (!aviGenerator->InitEngine()) {
			capturing = false;
			logOutput.Print(aviGenerator->GetLastErrorMessage());
			delete aviGenerator;
			aviGenerator = NULL;
		} else {
			LogObject() << "Recording avi to " << fileName << " size " << videoSizeX << " x " << videoSizeY;
		}

		SDL_ShowCursor(savedCursorMode);
		//aviGenerator->InitEngine() (avicap32.dll)? modifies the FPU control word.
		//Setting it back to default state.
		streflop_init<streflop::Simple>();
	}
}


void AviVideoCapturing::RenderFrame() {

	if (IsCapturing()) {
		globalRendering->lastFrameTime = 1.0f / GAME_SPEED;
		if (!aviGenerator->readOpenglPixelDataThreaded()) {
			StopCapturing();
		}
//		logOutput.Print("Saved avi frame size %i %i", ih->biWidth, ih->biHeight);
	}
}

#endif // defined AVI_CAPTURING

