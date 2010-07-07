/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VSYNC_H
#define VSYNC_H

class CVerticalSync {
	public:
		CVerticalSync();
		~CVerticalSync();

		void Init();
		void SetFrames(int frames);
		int  GetFrames() const { return frames; }

		void Delay() const;
		
	private:
		int frames;
};

extern CVerticalSync VSync;

#endif /* VSYNC_H */
