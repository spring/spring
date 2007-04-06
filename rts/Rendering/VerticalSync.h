#ifndef VSYNC_H
#define VSYNC_H


class CVerticalSync {
	public:
		CVerticalSync();
		~CVerticalSync();

		void Init();
		void SetFrames(int frames);
		int  GetFrames() const { return frames; }

		void Delay();
		
	private:
		int frames;
};


extern CVerticalSync VSync;


#endif /* VSYNC_H */
