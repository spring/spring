#include "Debug.h"


CDebug::CDebug(AIClasses *ai)
{
	this->ai=ai;
}	
CDebug::~CDebug()
{
}

void CDebug::MakeBWTGA(int* array,int xsize,int ysize,string filename,float curve)
{
	int totalsize = xsize * ysize;
	float* TGAArray = new float [totalsize];
	for(int i = 0; i < totalsize; i++){
		TGAArray[i]=array[i];
	}
	OutputBWTGA(TGAArray,xsize,ysize,filename,curve);
	delete [] TGAArray;
}

void CDebug::MakeBWTGA(float* array,int xsize,int ysize,string filename,float curve)
{
	int totalsize = xsize * ysize;
	float* TGAArray = new float [totalsize];
	for(int i = 0; i < totalsize; i++){
		TGAArray[i]=array[i];
	}
	OutputBWTGA(TGAArray,xsize,ysize,filename,curve);
	delete [] TGAArray;
}

void CDebug::MakeBWTGA(unsigned char* array,int xsize,int ysize,string filename,float curve)
{
	int totalsize = xsize * ysize;
	float* TGAArray = new float [totalsize];
	for(int i = 0; i < totalsize; i++){
		TGAArray[i]=array[i];
	}
	OutputBWTGA(TGAArray,xsize,ysize,filename,curve);
	delete [] TGAArray;
}

void CDebug::MakeBWTGA(bool* array,int xsize,int ysize,string filename,float curve)
{
	curve = 1;
	int totalsize = xsize * ysize;
	float* TGAArray = new float [totalsize];
	for(int i = 0; i < totalsize; i++){
		if(array[i])
			TGAArray[i]=255;
		else
			TGAArray[i]=0;
	}
	OutputBWTGA(TGAArray,xsize,ysize,filename,curve);
	delete [] TGAArray;
}

void CDebug::OutputBWTGA(float* array,int xsize,int ysize,string filename,float curve)
{
	float topvalue = 0;
	int totalsize = xsize * ysize;
	unsigned char* TGAArray = new unsigned char [totalsize];
	for(int i = 0; i < totalsize; i++){
		if(array[i] > topvalue){
			topvalue = array[i];
		}
	}
	if(topvalue != +0.0f && topvalue != -0.0f){
		if(curve == 1){
			topvalue = pow(topvalue, float(curve));
    		for(int i = 0; i < totalsize; i++){
				TGAArray[i] = int((pow(array[i],float(curve))*255) / topvalue);
			}
		}
		else if(curve != 0){
			for(int i = 0; i < totalsize; i++){
				TGAArray[i] = int((array[i]*255) / topvalue);
			}
		}	
	}
	string fullpath = string(TGAFOLDER) + filename + ".tga";
	char fullpath_buf[1000];
	strcpy(fullpath_buf, fullpath.c_str());
	ai->cb->GetValue(AIVAL_LOCATE_FILE_W, fullpath_buf);
	// open file
	FILE *fp=fopen(fullpath_buf, "wb");
	// fill & write header
	char Header[18];
	memset(Header, 0, sizeof(Header));
	Header[2] = 3;		// uncompressed gray-scale
	Header[12] = (char) (xsize & 0xFF);
	Header[13] = (char) (xsize >> 8);
	Header[14] = (char) (ysize & 0xFF);
	Header[15] = (char) (ysize >> 8);
	Header[16] = 8; // 8 bits/pixel
	Header[17] = 0x20;	
	fwrite(Header, 18, 1, fp);
	unsigned char out[1];
	for (int y=0; y < ysize; y++){
		for (int x=0; x < xsize; x++){
			out[0]=TGAArray[y * xsize + x];
			fwrite (out, 1, 1, fp);
		}
	}
	fclose(fp);
}
