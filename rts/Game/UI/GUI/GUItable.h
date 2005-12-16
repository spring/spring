// GUItable.h: interface for the GUItable class.
//
//////////////////////////////////////////////////////////////////////

#ifndef GUITABLE_H
#define GUITABLE_H

#include "GUIpane.h"
#include "Functor.h"
#include <string>
#include <vector>
class GUIscrollbar;

class GUItable : public GUIpane  
{
public:
	GUItable(int x, int y, int w, int h, const std::vector<std::string> *contents, Functor3<GUItable*, int, int>clicked);
	virtual ~GUItable();
	
	int GetSelected() const;
	void Select(int i,int button);
	virtual void ChangeList(const std::vector<std::string> *contents);
	void Scrolled(int pos);
	void SetSelect(int i,int button);
	
	struct HeaderInfo
	{
		HeaderInfo(const string& t, const float s)
		{
			title=t;
			size=s;
		}
		string title;
		float size;
		float position;
	};
	
	void SetHeader(const vector<HeaderInfo>& newHeaders);
protected:
	void PrivateDraw();

	bool MouseUpAction(int x, int y, int button);
	bool MouseDownAction(int x, int y, int button);

	int selected;  // which item is selected px
	int position;  // where in the list are we (pos)
	int length;    // how long is the list (px)
	int width;
	int rowHeight;

	int numLines;
	int numEntries;

	GUIscrollbar *bar;

	Functor3<GUItable*, int,int> click;
	vector<string> data;
	
	vector<HeaderInfo> header;
};

#endif // GUITABLE_H
