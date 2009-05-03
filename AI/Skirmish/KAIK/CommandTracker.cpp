#include "IncExternAI.h"

/*
struct AIClasses;
class CommandTracker {
	public:
		CommandTracker(AIClasses* aic):
			ai(aic),
			avgCommandsPerFrame(0.0f),
			maxCommandsPerFrame(0),
			peakCommandCountFrame(0),
			avgCommandSize(0.0f),
			totalCommandSize(0) {
		}
		~CommandTracker();

		static void GiveOrder(int, const Command*);

	private:
		AIClasses* ai;
		std::map<int, int> commandsPerFrame;

		int numCommands;

		float avgCommandsPerFrame;
		int maxCommandsPerFrame;
		int peakCommandFrame;

		float avgCommandSize;
		int totalCommandSize;
};

CommandTracker::CommandTracker(): peakCommandFrame(0) {
}
CommandTracker::~CommandTracker() {
	std::ofstream fs;
	std::stringstream ss;
	std::map<int, int>::const_iterator it;

	for (it = commandsPerFrame.begin(); it != commandsPerFrame.end(); it++) {
		ss << it->first << "\t" << it->second << "\n";
	}

	// TODO: make sure logger isn't deleted yet
	fs.open(ai->logger->GetLogName() + ".cmdstats");
	fs << ss.str();
	fs.close();
}

void CommandTracker::Update() {
	if ((ai->cb->GetCurrentFrame() % 1800) == 0) {
		avgCommandsPerFrame = numCommands / commandsPerFrame.size();
		avgCommandSize = totalCommandSize / numCommands;

		std::stringstream msg;
			msg << "[CommandTracker::Update()]\n";
			msg << "\taverage number of commands per frame: " << avgCommandsPerFrame << "\n";
			msg << "\tnumber of frames tracked: " << commandsPerFrame.size() << "\n";
			msg << "\tmaximum number of commands per frame: " << maxCommandsPerFrame << "\n";
			msg << "\tpeak command-count frame: " << peakCommandFrame << "\n";

		L(ai, msg.str());
	}
}

void CommandTracker::GiveOrder(int id, const Command* c) {
	const int f = ai->cb->GetCurrentFrame();

	if (commandsPerFrame.find(f) == commandsPerFrame.end()) {
		commandsPerFrame[f] = 1;
	} else {
		commandsPerFrame[f] += 1;
	}

	if (commandsPerFrame[f] > maxCommandsPerFrame) {
		maxCommandsPerFrame = commandsPerFrame[f];
		peakCommandFrame = f;
	}

	numCommands += 1;
	totalCommandSize += c->params.size();

	ai->cb->GiveOrder(id, c);
}
*/
