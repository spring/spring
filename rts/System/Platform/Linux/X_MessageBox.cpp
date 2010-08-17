/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Platform/errorhandler.h"

/**
 * @brief X message box function
 *
 * X clone of the Windows MessageBox() function, using the commandline
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
void X_MessageBox(const char *msg, const char *caption, unsigned int flags)
{
	char caption2[100];
	char msg2[1000];
	pid_t pid;
	int status, len;

	strncpy(caption2, caption, sizeof(caption2) - 2);
	strncpy(msg2, msg, sizeof(msg2) - 2);

	caption2[sizeof(caption2) - 2] = 0;
	msg2[sizeof(msg2) - 2] = 0;

   /* xmessage interprets some strings beginning with '-' as command-line
	* options. To avoid this we make sure all strings we pass to it have
	* newlines on the end. This is also useful for the fputs() case.
	*
	* kdialog and zenity always treat the argument after --error or --text
	* as a non-option argument.
    */

	len = strlen(caption2);
	if (len == 0 || caption2[len - 1] != '\n')
		strcat(caption2, "\n");

	len = strlen(msg2);
	if (len == 0 || msg2[len - 1] != '\n')
		strcat(msg2, "\n");

	/* fork a child */
	pid = fork();
	switch (pid) {

		case 0: /* child process */
		{
			const char* kde   = getenv("KDE_FULL_SESSION");
			const char* gnome = getenv("GNOME_DESKTOP_SESSION_ID");
			const char* type = "--error";

			if (gnome) {
				// --warning shows 2 buttons, so it should only be used with a true _warning_
				// ie. one which allows you to continue/cancel execution
// 				if (flags & MBF_EXCL) type = "--warning";
				if (flags & MBF_INFO) type = "--info";
				execlp("zenity", "zenity", "--title", caption2, type, "--text", msg2, (char*)NULL);
			}
			if (kde && strstr(kde, "true")) {
				if (flags & MBF_EXCL) type = "--sorry";
				if (flags & MBF_INFO) type = "--msgbox";
				execlp("kdialog", "kdialog", "--title", caption2, type, msg2, (char*)NULL);
			}
			execlp("xmessage", "xmessage", "-title", caption2, "-buttons", "OK:0", "-default", "OK", "-center", msg2, (char*)NULL);

			/* if execution reaches here, it means execlp failed */
			_exit(EXIT_FAILURE);
			break;
		}
		default: /* parent process */
		{
			waitpid(pid, &status, 0);
			if ((!WIFEXITED(status)) || (WEXITSTATUS(status) != 0)) /* ok button */
			{

			case -1: /* fork error */

				/* I kept this basically the same as the original
				console-only error reporting. */
				if (flags & MBF_INFO)
					fputs("Info: ", stderr);
				else if (flags & MBF_EXCL)
					fputs("Warning: ", stderr);
				else
					fputs("Error: ", stderr);
				fputs(caption2, stderr);
				fputs("  ", stderr);
				fputs(msg2, stderr);
			}

			break;
		}
	}
}
