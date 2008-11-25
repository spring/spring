#include "HeightLinePalette.h"

#include "ConfigHandler.h"


/** @brief Generates the height line palette.
Based on the configuration variable "ColorElev" (default: 1), it either
generates a colored palette or a grayscale palette. */

CHeightLinePalette::CHeightLinePalette()
{
	if(configHandler.Get("ColorElev",1)){
		for(int a=0;a<86;++a){
			heightLinePal[a*3+0]=255-a*3;
			heightLinePal[a*3+1]=a*3;
			heightLinePal[a*3+2]=0;
		}
		for(int a=86;a<172;++a){
			heightLinePal[a*3+0]=0;
			heightLinePal[a*3+1]=255-(a-86)*3;
			heightLinePal[a*3+2]=(a-86)*3;
		}
		for(int a=172;a<256;++a){
			heightLinePal[a*3+0]=(a-172)*3;
			heightLinePal[a*3+1]=0;
			heightLinePal[a*3+2]=255-(a-172)*3;
		}
	} else {
		for(int a=0;a<29;++a){
			heightLinePal[a*3+0]=255-a*8;
			heightLinePal[a*3+1]=255-a*8;
			heightLinePal[a*3+2]=255-a*8;
		}
		for(int a=29;a<256;++a){
			heightLinePal[a*3+0]=a;
			heightLinePal[a*3+1]=a;
			heightLinePal[a*3+2]=a;
		}
	}
}
