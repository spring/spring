#ifndef CGROUND
    #define CGROUND_H
    

class cGround
{
public:
	//default Constructor
	cGround()
	
	//destructor
	~cGround

private:
	//heightmapReference
	struct collissionDetails primitveCollissionCheck(&cPObject testObject)
	{
	float radiusHalfed=(int)testObject->simpleSphericCollision.radius/2;
	for (int i= (int) testObject->m_Position.x-radiusHalfed;i <(int) testObject->m_Position.x+radiusHalfed;i++)
		{
			for (int j= (int) testObject->m_Position.z-radiusHalfed;j <(int) testObject->m_Position.z+radiusHalfed;j++)
			{
				
			}
		}
	//@Todo: Return collissionDetails Struct 	
	};
	
	
};

//locked direction is zone surrounded by rad coords

#endif // CGROUND_H
