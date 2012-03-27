/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Shameless rip of get_executable_name implementation of Allegro
 * for various platforms.
 *
 * http://alleg.sourceforge.net
 */

/* Allegro license, from http://alleg.sourceforge.net/license.html.
This license applies to the code in this file.

"Allegro is gift-ware. It was created by a number of people working in cooperation, and is given to you freely as a gift. You may use, modify,
redistribute, and generally hack it about in any way you like, and you do not have to give us anything in return.

However, if you like this product you are encouraged to thank us by making a return gift to the Allegro community. This could be by writing an
add-on package, providing a useful bug report, making an improvement to the library, or perhaps just releasing the sources of your program so that
other people can learn from them. If you redistribute parts of this code or make a game using it, it would be nice if you mentioned Allegro
somewhere in the credits, but you are not required to do this. We trust you not to abuse our generosity.

By Shawn Hargreaves, 18 October 1998.

DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE
SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
*/

#ifdef WIN32

#include <windows.h>

/* sys_directx_get_executable_name:
 *  Returns full path to the current executable.
 */
void get_executable_name(char *output, int size)
{
	if (!GetModuleFileName(GetModuleHandle(NULL), output, size))
		*output = 0;
}

#else

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* If we want we can add System V procfs support later
by simply defining ALLEGRO_HAVE_SV_PROCFS. */
#ifdef ALLEGRO_HAVE_SV_PROCFS
#include <sys/procfs.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

/* _find_executable_file:
 *  Helper function: searches path and current directory for executable.
 *  Returns 1 on succes, 0 on failure.
 */
static int _find_executable_file(const char *filename, char *output, int size)
{
	char *path;

	/* If filename has an explicit path, search current directory */
	if (strchr (filename, '/')) {
		if (filename[0] == '/') {
			/* Full path; done */
			strncpy(output, filename, size);
			output[size - 1] = 0;
			return 1;
		}
		else {
			struct stat finfo;
			int len;

			/* Prepend current directory */
			const char* result = getcwd(output, size);
			if (!result) output[0] = 0;

			len = strlen(output);
			output[len] = '/';
			strncpy(output+len+1, filename, size-len-1);
			output[size - 1] = 0;

			if ((stat(output, &finfo)==0) && (!S_ISDIR (finfo.st_mode)))
				return 1;
		}
	}
	/* If filename has no explicit path, but we do have $PATH, search there */
	else if ((path = getenv("PATH"))) {
		char *start = path, *end = path, *buffer = NULL, *temp;
		struct stat finfo;

		while (*end) {
			end = strchr (start, ':');
			if (!end) end = strchr (start, '\0');

			/* Resize `buffer' for path component, slash, filename and a '\0' */
			temp = realloc (buffer, end - start + 1 + strlen (filename) + 1);
			if (temp) {
				buffer = temp;

				strcpy (buffer, start);
				*(buffer + (end - start)) = '/';
				strcpy (buffer + (end - start) + 1, filename);

				if ((stat(buffer, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
					strncpy(output, buffer, size);
					output[size - 1] = 0;
					free (buffer);
					return 1;
				}
			} /* else... ignore the failure; `buffer' is still valid anyway. */

			start = end + 1;
		}
		/* Path search failed */
		free (buffer);
	}

	return 0;
}

/* _unix_get_executable_name:
 *  Return full path to the current executable, use proc fs if available.
 */
void get_executable_name(char *output, int size)
{
#ifdef ALLEGRO_HAVE_SV_PROCFS
	struct prpsinfo psinfo;
	int fd;
#endif
#if defined ALLEGRO_HAVE_SV_PROCFS || defined ALLEGRO_SYS_GETEXECNAME
	char *s;
#endif
	char linkname[1024];
	char filename[1024];
	struct stat finfo;
	FILE *pipe;
	pid_t pid;
	int len;

#ifdef ALLEGRO_HAVE_GETEXECNAME
	s = getexecname();
	if (s) {
		if (s[0] == '/') {   /* Absolute path */
			strncpy(output, s, size);
			output[size - 1] = 0;
			return;
		}
		else {               /* Not an absolute path */
			if (_find_executable_file(s, output, size))
				return;
		}
	}
	s = NULL;
#endif

	/* We need the PID in order to query procfs */
	pid = getpid();

	/* Try a Linux-like procfs */
	/* get symolic link to executable from proc fs */
	sprintf (linkname, "/proc/%d/exe", pid);
	if (stat (linkname, &finfo) == 0) {
		len = readlink (linkname, filename, sizeof(filename)-1);
		if (len>-1) {
			filename[len] = '\0';

			strncpy(output, filename, size);
			output[size - 1] = 0;
			return;
		}
   }

   /* Use System V procfs calls if available */
#ifdef ALLEGRO_HAVE_SV_PROCFS
	sprintf (linkname, "/proc/%d/exe", pid);
	fd = open(linkname, O_RDONLY);
	if (fd >= 0) {
		ioctl(fd, PIOCPSINFO, &psinfo);
		close(fd);

		/* Use argv[0] directly if we can */
#ifdef ALLEGRO_HAVE_PROCFS_ARGCV
	if (psinfo.pr_argv && psinfo.pr_argc) {
		if (_find_executable_file(psinfo.pr_argv[0], output, size))
			return;
	}
	else
#endif
	{
		/* Emulate it */
		/* We use the pr_psargs field to find argv[0]
		 * This is better than using the pr_fname field because we need
		 * the additional path information that may be present in argv[0]
		*/

		/* Skip other args */
		s = strchr(psinfo.pr_psargs, ' ');
		if (s) s[0] = '\0';
		if (_find_executable_file(psinfo.pr_psargs, output, size))
			 return;
	}

	/* Try the pr_fname just for completeness' sake if argv[0] fails */
	if (_find_executable_file(psinfo.pr_fname, output, size))
		return;
	}
#endif

	/* Last resort: try using the output of the ps command to at least find */
	/* the name of the file if not the full path */
	sprintf (filename, "ps -p %d", pid);
	pipe = popen(filename, "r");
	if (pipe) {
		/* The first line of output is a header */
		const char* result = fgets(linkname, sizeof(linkname), pipe);
		if (!result) linkname[0] = 0;

		/* The information we want is in the last column; find it */
		len = strlen(linkname);
		while (linkname[len] != ' ' && linkname[len] != '\t')
			len--;

		/* The second line contains the info we want */
		result = fgets(linkname, sizeof(linkname), pipe);
		if (!result) { linkname[0] = 0; len = 0; }
		pclose(pipe);

		/* Treat special cases: filename between [] and - for login shell */
		if (linkname[len] == '-')
			len++;

		/* Now, the filename should be in the last column */
		strcpy(filename, linkname+len+1);

		if (_find_executable_file(filename, output, size))
			return;

		/* Just return the output from ps... */
		strncpy(output, filename, size);
		output[size - 1] = 0;
		return;
	}

#ifdef ALLEGRO_WITH_MAGIC_MAIN
	/* Try the captured argv[0] */
	if (_find_executable_file(__crt0_argv[0], output, size))
		return;
#endif

	/* Give up; return empty string */
	if (size > 0)
		*output = '\0';
}

#endif
