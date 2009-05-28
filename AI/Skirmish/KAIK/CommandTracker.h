#ifndef KAIK_COMMANDTRACKER_HDR
#define KAIK_COMMANDTRACKER_HDR

struct AIClasses;
struct Command;
class CCommandTracker {
	public:
		CCommandTracker(AIClasses* aic):
			ai(aic),
			maxCmdsPerFrame(0),
			peakCmdFrame(0),
			avgCmdSize(0.0f),
			totalCmdSize(0),
			totalNumCmds(0) {
		}
		~CCommandTracker();

		void Update(int);
		void GiveOrder(int, Command*);

	private:
		AIClasses* ai;
		std::map<int, int> cmdsPerFrame;

		int   maxCmdsPerFrame;
		int   peakCmdFrame;

		float avgCmdSize;
		int   totalCmdSize;
		int   totalNumCmds;
};

#endif
