#ifndef SKYBOX_H
#define SKYBOX_H

#include "BaseSky.h"
#include <string>

class CSkyBox : public CBaseSky
{
public:
	CSkyBox(std::string texture);
	~CSkyBox(void);
	void Update();
	void DrawSun(void);

	void Draw();

private:
	unsigned int tex;
	unsigned int displist;
};

#endif
