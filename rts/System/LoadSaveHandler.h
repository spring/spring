#ifndef LOADSAVEHANDLER_H
#define LOADSAVEHANDLER_H

#include <string>

class CLoadInterface;

class CLoadSaveHandler
{
public:
	CLoadSaveHandler(void);
	~CLoadSaveHandler(void);
	void SaveGame(std::string file);
	void LoadGame(std::string file);
	void LoadGame2(void);

	std::ifstream* ifs;
	CLoadInterface* load;
};


#endif /* LOADSAVEHANDLER_H */
