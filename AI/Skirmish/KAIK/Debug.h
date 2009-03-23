#ifndef KAIK_DEBUG_HDR
#define KAIK_DEBUG_HDR

#include <string>

struct AIClasses;

class CDebug {
	public:
		CDebug(AIClasses* ai);
		~CDebug();

		void MakeBWTGA(int* array, int xsize, int ysize, std::string filename, float curve = 1);
		void MakeBWTGA(float* array, int xsize, int ysize, std::string filename, float curve = 1);
		void MakeBWTGA(unsigned char* array, int xsize, int ysize, std::string filename, float curve = 1);
		void MakeBWTGA(bool* array, int xsize, int ysize, std::string filename, float curve = 1);

	private:
		AIClasses* ai;
		void OutputBWTGA(float* array, int xsize, int ysize, std::string filename, float curve);
};

#endif
