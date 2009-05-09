#include "InputReceiver.h"

//TOD add support for resigning without selfdestruction
class CQuitBox :
	public CInputReceiver
{
public:
	CQuitBox(void);
	~CQuitBox(void);

	void Draw(void);

	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	bool MousePress(int x, int y, int button);
	void MouseRelease(int x,int y,int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	bool KeyPressed(unsigned short key, bool isRepeat);

private:
	ContainerBox box;

	// in order of appereance ...
	ContainerBox resignBox;
	ContainerBox saveBox;
	ContainerBox giveAwayBox;
	ContainerBox teamBox;
	ContainerBox quitBox;
	ContainerBox cancelBox;

	int shareTeam;
	bool noAlliesLeft;

	bool moveBox;
};

