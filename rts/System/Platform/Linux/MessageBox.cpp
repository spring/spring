/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

#include "System/Platform/MessageBox.h"
#include "System/Platform/errorhandler.h"

namespace Platform {

/**
 * @brief X message box function
 *
 * X clone of the Windows' MessageBox() function, using the commandline
 * dialog creation tools zenity, kdialog or xmessage as a workhorse.
 *
 * If DESKTOP_SESSION is set to "gnome" or "kde", it attempts to use
 * zenity (Gnome) or kdialog (KDE).
 *
 * If DESKTOP_SESSION is not set, or if executing zenity or kdialog failed,
 * it attempts to use the generic (but ugly!) xmessage.
 *
 * As a last resort (if forking failed, none of the above mentioned programs
 * could be found, or the used program returned an error) the message is
 * written to stderr.
 */
void MsgBox(const char* message, const char* caption, unsigned int flags)
{
	char cap[1024];
	char msg[1024];
	pid_t pid;
	int status;
	int len;

	strncpy(cap, caption, sizeof(cap) - 2);
	strncpy(msg, message, sizeof(msg) - 2);

	cap[sizeof(cap) - 2] = 0;
	msg[sizeof(msg) - 2] = 0;

	/*
	 * xmessage interprets some strings beginning with '-' as command-line
	 * options. To avoid this, we make sure all strings we pass to it have
	 * newlines on the end. This is also useful for the fputs() case.
	 *
	 * kdialog and zenity always treat the argument after --error or --text
	 * as a non-option argument.
	 */

	if ((len = strlen(cap)) == 0 || cap[len - 1] != '\n')
		strcat(cap, "\n");

	if ((len = strlen(msg)) == 0 || msg[len - 1] != '\n')
		strcat(msg, "\n");

	// fork a child
	switch (pid = fork()) {
		case 0: {
			// child process
			const char* kde   = getenv("KDE_FULL_SESSION");
			const char* gnome = getenv("GNOME_DESKTOP_SESSION_ID");
			const char* type = "--error";

			if (gnome) {
				if (flags & MBF_EXCL) {
					// --warning shows 2 buttons, so it should only be used with a true _warning_
					// ie. one which allows you to continue/cancel execution
					//type = "--warning";
					type = "--error";
				}
				else if (flags & MBF_CRASH) {
					type = "--error";
				}
				else if (flags & MBF_INFO) {
					type = "--info";
				}
				execlp("zenity", "zenity", "--title", cap, type, "--text", msg, (char*)nullptr);
			}
			if (kde && strstr(kde, "true")) {
				if (flags & MBF_CRASH) {
					type = "--error";
				}
				else if (flags & MBF_EXCL) {
					type = "--sorry";
				}
				else if (flags & MBF_INFO) {
					type = "--msgbox";
				}
				execlp("kdialog", "kdialog", "--title", cap, type, msg, (char*)nullptr);
			}
			execlp("xmessage", "xmessage", "-title", cap, "-buttons", "OK:0", "-default", "OK", "-center", msg, (char*)nullptr);

			// if execution reaches here, it means execlp failed
			_exit(EXIT_FAILURE);
		} break;

		default: {
			// parent process
			waitpid(pid, &status, 0);

			const bool okButton = (!WIFEXITED(status) || (WEXITSTATUS(status) != 0));

			if (!okButton)
				break;
		}

		case -1: {
			// fork error
			if (flags & MBF_INFO) {
				fputs("Info: ", stderr);
			} else if (flags & MBF_EXCL) {
				fputs("Warning: ", stderr);
			} else {
				fputs("Error: ", stderr);
			}
			fputs(cap, stderr);
			fputs("  ", stderr);
			fputs(msg, stderr);
		} break;
	}
}

} //namespace Platform
