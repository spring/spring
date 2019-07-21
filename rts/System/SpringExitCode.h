/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_EXIT_CODE_H
#define SPRING_EXIT_CODE_H

namespace spring {
	enum {
		EXIT_CODE_FAILURE = -1002, // SpringApp::Run
		EXIT_CODE_DESYNC  = -1001, // GameServer::CheckSync
		EXIT_CODE_SUCCESS =     0,
		EXIT_CODE_TIMEOUT =  1001, // PreGame::UpdateClientNet
		EXIT_CODE_FORCED  =  1002, // Game::Load
	};

	// only here for validation tests
	extern int exitCode;
};

#endif

