/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VSYNC_H
#define VSYNC_H

class CVerticalSync {
public:
	void WrapNotifyOnChange();
	void WrapRemoveObserver();

	void Toggle();
	void SetInterval();
	void SetInterval(int i);
	void ConfigNotify(const std::string& key, const std::string& value);
	int  GetInterval() const { return interval; }

	static CVerticalSync* GetInstance();

private:
	// must start at a value that can not occur in config; see SetInterval
	int interval = -1000;
};

#define verticalSync (CVerticalSync::GetInstance())

#endif /* VSYNC_H */
