#if !defined(GUILABEL_H)
#define GUILABEL_H

#include "GUIframe.h"
#include <string>

class GUIlabel : public GUIframe, public GUIcaption
{
public:
	GUIlabel(const int x1, const int y1, int w1, int h1, const std::string& caption);
	virtual ~GUIlabel();
	
	string& GetCaption();
	void SetCaption(const string& c);

protected:
	void PrivateDraw();
	void BuildList();
	
	string caption;
	GLuint displayList;
};

#endif // !defined(GUILABEL_H)
