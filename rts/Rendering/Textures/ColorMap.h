#ifndef COLORMAP_H
#define COLORMAP_H

#include <string>
#include <vector>

//Simple class to interpolate between 32bit RGBA colors
class CColorMap
{
public:
	CColorMap(void);
	CColorMap(std::vector<float> &vec);  //loads from a float vector
	CColorMap(std::string filename);	//loads from a filename
	CColorMap(const unsigned char *buf, int num);	//loads from a char buffer as one dimensional array
	~CColorMap(void);

	//load colormap from a bitmap
	void LoadFromBitmapFile(std::string filename);

	//takes a vector with float value 0.0-1.0, 4 values makes up a rgba color
	void LoadFromFloatVector(std::vector<float> &vec);

	//load from a string containing a numbef or float values example: "1.0 0.5 1.0 ... "
	void LoadFromFloatString(std::string fstring);

	//to load default values...
	void Load8f(float r1,float g1,float b1,float a1,float r2,float g2,float b2,float a2);
	void Load12f(float r1,float g1,float b1,float a1,float r2,float g2,float b2,float a2,float r3,float g3,float b3,float a3);

	//color - buffer with room for 4 bytes
	//pos - value between 0 and 1, returns pointer to color 
	unsigned char* GetColor(unsigned char *color, float pos);

protected:
	unsigned char *map;
	int xsize, nxsize;
	int ysize, nysize;

	void LoadMap(const unsigned char *buf, int num);
};

#endif