#ifndef PREGAME_H
#define PREGAME_H
/*pragma once removed*/
#include "GameController.h"
#include <string>

class CglList;

class CPreGame : public CGameController
{
public:
	CPreGame(bool server, const std::string& demo);
	virtual ~CPreGame(void);

	CglList* showList;

	bool Draw(void);
	int KeyPressed(unsigned short k,bool isRepeat);
	bool Update(void);

	bool server;
	bool waitOnAddress;
	bool waitOnScript;
	bool waitOnMap;
	bool allReady;
	bool saveAddress;

	std::string mapName;
	int mapId;

	void UpdateClientNet(void);
	unsigned char inbuf[16000];	//buffer space for incomming data
	int inbufpos;								//where in the input buffer we are
	int inbuflength;						//last byte in input buffer
	void ShowMapList(void);
	static void SelectMap(std::string s);
};

extern CPreGame* pregame;

#endif /* PREGAME_H */
