#include <fstream>
#include <sstream>

#include "IncExternAI.h"
#include "IncGlobalAI.h"

CCommandTracker::~CCommandTracker() {
	std::ofstream fs;
	std::stringstream ss;
	std::string s = ai->logger->GetLogName() + ".cmdstats";
	std::map<int, int>::const_iterator it;

	for (it = cmdsPerFrame.begin(); it != cmdsPerFrame.end(); it++) {
		ss << it->first << "\t" << it->second << "\n";
	}

	fs.open(s.c_str(), std::ios::out);
	fs << ss.str();
	fs.close();
}

void CCommandTracker::Update(int currFrame) {
	if ((currFrame % 1800) == 0 && !cmdsPerFrame.empty()) {
		const int   numFrames        = cmdsPerFrame.size();
		const float avgCmdsRegFrames = totalNumCmds / float(numFrames);
		const float avgCmdsAllFrames = totalNumCmds / float(currFrame);
		const float avgCmdSize       = totalCmdSize / float(totalNumCmds);

		std::stringstream msg;
			msg << "[CCommandTracker::Update()] frame " << currFrame << "\n";
			msg << "\tnumber of frames registered:                    " << numFrames        << "\n";
			msg << "\t(avg.) number of commands (registered frames):  " << avgCmdsRegFrames << "\n";
			msg << "\t(avg.) number of commands (all elapsed frames): " << avgCmdsAllFrames << "\n";
			msg << "\t(avg.) number of parameters per command:        " << avgCmdSize       << "\n";
			msg << "\t(max.) number of commands, peak frame:          "
				<< maxCmdsPerFrame << ", "
				<< peakCmdFrame    << "\n";

		L(ai, msg.str());
	}
}

void CCommandTracker::GiveOrder(int id, Command* c) {
	const int f = ai->cb->GetCurrentFrame();

	if (cmdsPerFrame.find(f) == cmdsPerFrame.end()) {
		cmdsPerFrame[f] = 1;
	} else {
		cmdsPerFrame[f] += 1;
	}

	if (cmdsPerFrame[f] > maxCmdsPerFrame) {
		maxCmdsPerFrame = cmdsPerFrame[f];
		peakCmdFrame    = f;
	}

	totalNumCmds += 1;
	totalCmdSize += c->params.size();

	ai->cb->GiveOrder(id, c);
}
