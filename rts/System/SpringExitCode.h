/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_EXIT_CODE_H
#define SPRING_EXIT_CODE_H

namespace spring {
	enum {
		EXIT_CODE_CRASHED = -1003, // ErrorHandler::ExitProcess
		EXIT_CODE_NOINIT  = -1002, // SpringApp::Run
		EXIT_CODE_DESYNC  = -1001, // GameServer::CheckSync
		EXIT_CODE_SUCCESS =     0,
		EXIT_CODE_FAILURE =     1, // SpringApp::ParseCmdLine
		EXIT_CODE_TIMEOUT =  1001, // PreGame::UpdateClientNet
		EXIT_CODE_NOLOAD  =  1002, // Game::Load
		EXIT_CODE_KILLED  =  1003, // CrashHandler::ForcedExit
	};

	// only here for validation tests
	extern int exitCode;
};

#endif

