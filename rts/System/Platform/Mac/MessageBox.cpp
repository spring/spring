/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/MessageBox.h"

#include <CFBase.h>
#include <CFString.h>
#include <CFUserNotification.h>

namespace Platform {

/**
 * @brief message box function
 *
 * MacOSX clone of the Windows' MessageBox() function.
 */
void MessageBox(const std::string& message, const std::string& caption, const unsigned int& flags)
{
	CFStringRef cf_caption = CFStringCreateWithCString(NULL, caption.c_str(), caption.size());
	CFStringRef cf_message = CFStringCreateWithCString(NULL, message.c_str(), message.size());

	CFOptionFlags cfFlags = 0;
	if (flags & MBF_EXCL)  cfFlags |= kCFUserNotificationCautionAlertLevel;
	if (flags & MBF_INFO)  cfFlags |= kCFUserNotificationPlainAlertLevel;
	if (flags & MBF_CRASH) cfFlags |= kCFUserNotificationStopAlertLevel;

	CFUserNotificationDisplayNotice(
		0,    // timeout
		cfFlags,
		NULL, // icon url (use default depending on flags)
		NULL, // sound url
		NULL, // localization url
		cf_caption, // caption text 
		cf_message, // message text
		NULL  // button text (use default "ok")
	);

	// Clean up the strings
	CFRelease(cf_caption);
	CFRelease(cf_message);
}

}; //namespace Platform
