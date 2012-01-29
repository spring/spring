/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if !defined(DEDICATED) && !defined(HEADLESS)
/**
 * Will pop up an message window
 * @param  msg     the main text, describing the error
 * @param  caption will appear in the title bar of the error window
 * @param  flags   @see errorhandler.h
 */
void X_MessageBox(const char* msg, const char* caption, unsigned int flags);
#endif

