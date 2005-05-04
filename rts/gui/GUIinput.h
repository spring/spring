#if !defined(GUIINPUT_H)
#define GUIINPUT_H

#include "GUIframe.h"
#include "Functor.h"
#include <string>

class GUIinput : public GUIframe, public GUIcaption
{
public:
	GUIinput(const int x1, const int y1, int w1, int h1, Functor1<const string&> s);
	virtual ~GUIinput();

	void SetCaption(const string& caption1) { caption=caption1; }
protected:
	string caption;
	string userInput;
	void PrivateDraw();
	
	bool KeyAction(const int key);
	bool CharacterAction(const char key);
	
	bool EventAction(const std::string& event);
	
	Functor1<const string&> enter;
};

#endif // !defined(GUIINPUT_H)
