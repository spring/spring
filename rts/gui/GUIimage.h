   // GUIimage.h: interface for the GUIimage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIIMAGE_H__9C3E486C_3635_47CC_9697_51644E0207A7__INCLUDED_)
#define AFX_GUIIMAGE_H__9C3E486C_3635_47CC_9697_51644E0207A7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"

class GUIimage : public GUIframe  
{
public:
	GUIimage(int x, int y, int w, int h, const std::string& image);
	virtual ~GUIimage();
	
	void SetImage(const std::string& image);
protected:
	GLuint texture;

	void PrivateDraw();
};

#endif // !defined(AFX_GUIIMAGE_H__9C3E486C_3635_47CC_9697_51644E0207A7__INCLUDED_)
