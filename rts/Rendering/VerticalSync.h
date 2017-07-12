/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VSYNC_H
#define VSYNC_H

class CVerticalSync {
public:
	CVerticalSync();
	~CVerticalSync();

	void SetInterval();
	void SetInterval(int i, bool updateConf = true);
	void ConfigNotify(const std::string& key, const std::string& value);
	int  GetInterval() const { return interval; }

	void Toggle();
	void Delay() const;

	static CVerticalSync* GetInstance();

private:
	int interval;
};

#define verticalSync (CVerticalSync::GetInstance())

#endif /* VSYNC_H */
