#ifndef PREGAME_H
#define PREGAME_H

#include "GameController.h"
#include <string>

class CglList;

class CPreGame : public CGameController
{
public:
	enum State {
		UNKNOWN,
		WAIT_ON_ADDRESS,
		WAIT_ON_SCRIPT,
		WAIT_ON_MAP,
		WAIT_ON_MOD,
		ALL_READY,
	};
	CPreGame(bool server, const std::string& demo);
	virtual ~CPreGame();

	CglList* showList;

	bool Draw();
	int KeyPressed(unsigned short k,bool isRepeat);
	bool Update();

	bool server;
	State state;
	bool saveAddress;

	std::string mapName;
	std::string modName;

	void UpdateClientNet();
	unsigned char inbuf[16000];	//buffer space for incomming data
	int inbufpos;								//where in the input buffer we are
	int inbuflength;						//last byte in input buffer

	unsigned GetMapChecksum() const;
	unsigned GetModChecksum() const;

private:

	void ShowScriptList();
	void ShowMapList();
	void ShowModList();
	static void SelectScript(std::string s);
	static void SelectMap(std::string s);
	static void SelectMod(std::string s);
};

extern CPreGame* pregame;

#endif /* PREGAME_H */
