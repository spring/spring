/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <string>
#include <functional>

void ShowSplashScreen(
	const std::string& splashScreenFile,
	const std::string& springVersionStr,
	const std::function<bool()>& testDoneFunc
);

#endif

