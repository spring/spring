/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/MessageBox.h"

#if 0 //!defined(DEDICATED) && !defined(HEADLESS)
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFUserNotification.h>
#endif

namespace Platform {

/**
 * @brief message box function
 *
 * MacOSX clone of the Windows' MessageBox() function.
 */
void MsgBox(const std::string& message, const std::string& caption, const unsigned int& flags)
{
#if 0 //!defined(DEDICATED) && !defined(HEADLESS)
	CFStringRef cf_caption = CFStringCreateWithCString(NULL, caption.c_str(), caption.size());
	CFStringRef cf_message = CFStringCreateWithCString(NULL, message.c_str(), message.size());

	CFOptionFlags cfFlags = 0;
	CFOptionFlags result;
	if (flags & MBF_EXCL)  cfFlags |= kCFUserNotificationCautionAlertLevel;
	if (flags & MBF_INFO)  cfFlags |= kCFUserNotificationPlainAlertLevel;
	if (flags & MBF_CRASH) cfFlags |= kCFUserNotificationStopAlertLevel;

	CFUserNotificationDisplayAlert(
		0,    // timeout
		cfFlags,
		NULL, // icon url (use default depending on flags)
		NULL, // sound url
		NULL, // localization url
		cf_caption, // caption text 
		cf_message, // message text
		NULL,  // button text (use default "ok")
		NULL, // alternate button title
		NULL, // other button title
		&result // result
	);

	// Clean up the strings
	CFRelease(cf_caption);
	CFRelease(cf_message);
#endif
}

}; //namespace Platform
