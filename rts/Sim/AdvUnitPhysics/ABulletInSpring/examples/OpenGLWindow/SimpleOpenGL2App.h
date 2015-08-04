#ifndef SIMPLE_OPENGL2_APP_H
#define SIMPLE_OPENGL2_APP_H

#include "../CommonInterfaces/CommonGraphicsAppInterface.h"

class SimpleOpenGL2App : public CommonGraphicsApp
{
protected:
	struct SimpleOpenGL2AppInternalData*	m_data;

public:
	SimpleOpenGL2App(const char* title, int width, int height);
	virtual ~SimpleOpenGL2App();

	virtual void drawGrid(DrawGridData data=DrawGridData());
	virtual void setUpAxis(int axis);
	virtual int getUpAxis() const;
	
	virtual void swapBuffer();
	virtual void drawText( const char* txt, int posX, int posY);

	virtual int	registerCubeShape(float halfExtentsX,float halfExtentsY, float halfExtentsZ)
	{
		return 0;
	}
	virtual int	registerGraphicsSphereShape(float radius, bool usePointSprites, int largeSphereThreshold, int mediumSphereThreshold)
	{
		return 0;
	}
    virtual void drawText3D( const char* txt, float posX, float posZY, float posZ, float size);
    virtual void registerGrid(int xres, int yres, float color0[4], float color1[4]);

    
};
#endif //SIMPLE_OPENGL2_APP_H