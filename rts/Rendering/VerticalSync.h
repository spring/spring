/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VSYNC_H
#define VSYNC_H

class CVerticalSync {
	public:
		CVerticalSync();
		~CVerticalSync();

		void Init();
		void SetInterval(int interval);
		int  GetInterval() const { return interval; }

		void Delay() const;
		
	private:
		int interval;
};

extern CVerticalSync VSync;

#endif /* VSYNC_H */
