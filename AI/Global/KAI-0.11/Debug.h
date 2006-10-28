#pragma once
#include "GlobalAI.h"

class CDebug
{
public:
	CDebug(AIClasses *ai);
	virtual ~CDebug();

	
	void MakeBWTGA(int* array,int xsize,int ysize,string filename,float curve = 1);
	void MakeBWTGA(float* array,int xsize,int ysize,string filename,float curve = 1);
	void MakeBWTGA(unsigned char* array,int xsize,int ysize,string filename,float curve = 1);
	void MakeBWTGA(bool* array,int xsize,int ysize,string filename,float curve = 1);




private:
	AIClasses *ai;	
	void OutputBWTGA(float* array,int xsize,int ysize,string filename,float curve);
};