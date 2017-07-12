/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VSYNC_H
#define VSYNC_H

class CVerticalSync {
public:
	CVerticalSync(): interval(0) {}

	void WrapNotifyOnChange();
	void WrapRemoveObserver();

	void Toggle();
	void SetInterval();
	void SetInterval(int i);
	void ConfigNotify(const std::string& key, const std::string& value);
	int  GetInterval() const { return interval; }

	static CVerticalSync* GetInstance();

private:
	int interval;
};

#define verticalSync (CVerticalSync::GetInstance())

#endif /* VSYNC_H */
