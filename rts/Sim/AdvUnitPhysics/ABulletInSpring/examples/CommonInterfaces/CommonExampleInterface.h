

#ifndef COMMON_EXAMPLE_INTERFACE_H
#define COMMON_EXAMPLE_INTERFACE_H

struct CommonExampleOptions
{
	struct GUIHelperInterface*	m_guiHelper;

	//Those are optional, some examples will use them others don't. Each example should work with them being 0.
	int			m_option;
	const char* m_fileName;

	
	CommonExampleOptions(struct GUIHelperInterface*	helper, int option=0)
		:m_guiHelper(helper),
		m_option(option),
		m_fileName(0)
	{
	}

};

class CommonExampleInterface
{
public:

	typedef class CommonExampleInterface* (CreateFunc)(CommonExampleOptions& options);

	virtual ~CommonExampleInterface()
	{
	}

	
	
	virtual void    initPhysics()=0;
	virtual void    exitPhysics()=0;
	virtual void	stepSimulation(float deltaTime)=0;
	virtual void	renderScene()=0;
	virtual void	physicsDebugDraw(int debugFlags)=0;//for now we reuse the flags in Bullet/src/LinearMath/btIDebugDraw.h
	//reset camera is only called when switching demo. this way you can restart (initPhysics) and watch in a specific location easier
	virtual void	resetCamera(){};
	virtual bool	mouseMoveCallback(float x,float y)=0;
	virtual bool	mouseButtonCallback(int button, int state, float x, float y)=0;
	virtual bool	keyboardCallback(int key, int state)=0;

};



#endif //COMMON_EXAMPLE_INTERFACE_H
