/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCOPED_FILE_LOCK
#define SCOPED_FILE_LOCK

/**
 * @brief POSIX file locking class
 * Does nothing on WIN32.
 */
class ScopedFileLock
{
public:
	ScopedFileLock(int fd, bool write);
	~ScopedFileLock();
private:
	int filedes;
};

#endif // SCOPED_FILE_LOCK
