#include "CrashHandler.h"

#ifdef _WIN32
#include "Win/CrashHandler.h"
#include "Platform/Win/seh.h"

namespace CrashHandler {
	void Install()
	{
		Win32::Install();
		InitializeSEH();
	};
	
	void Remove()
	{
		Win32::Remove();
	}
};

#else

#include <string>
#include <vector>
#include <signal.h>
#include <execinfo.h>

#include "LogOutput.h"
#include "errorhandler.h"
#include "Game/GameVersion.h"

namespace CrashHandler {
	
	void HandleSignal(int signal)
	{
		logOutput.RemoveAllSubscribers();
		LogObject log;
		std::string error;
		if (signal == SIGSEGV)
			error = "Segmentation fault (SIGSEGV)";
		else if (signal == SIGILL)
			error = "Illegal instruction (SIGILL)";
		else if (signal == SIGPIPE)
			error = "Broken pipe (SIGPIPE)";
		else if (signal == SIGIO)
			error = "IO-Error (SIGIO)";
		log << error << " in spring " << SpringVersion::GetFull() << "\nStacktrace:\n";

		std::vector<void*> buffer(128);
		const int numptr = backtrace(&buffer[0], buffer.size()); // stack pointers
		char ** strings = backtrace_symbols(&buffer[0], numptr); // give them meaningfull names
		if (strings == NULL)
		{
			log << "Unable to create stacktrace\n";
		}
		else
		{
			for (int j = 0; j < numptr; j++)
			{
				log << strings[j] << "\n";
			}
			delete strings;
		}
		logOutput.End();  // Stop writing to log.
		ErrorMessageBox(error, "Spring crashed", 0);
	};
	
	void Install()
	{
		signal(SIGSEGV, HandleSignal); // segmentation fault
		signal(SIGILL, HandleSignal); // illegal instruction
		signal(SIGPIPE, HandleSignal); // sigpipe, maybe some network error
		signal(SIGIO, HandleSignal); // who knows?
	};
	
	void Remove()
	{
		signal(SIGSEGV, SIG_DFL);
		signal(SIGILL, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);
		signal(SIGIO, SIG_DFL);
	}
};

#endif