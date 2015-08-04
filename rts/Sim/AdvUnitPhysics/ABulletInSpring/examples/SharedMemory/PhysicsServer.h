#ifndef PHYSICS_SERVER_SHARED_MEMORY_H
#define PHYSICS_SERVER_SHARED_MEMORY_H

class PhysicsServerSharedMemory
{
	struct PhysicsServerInternalData* m_data;

protected:

	void	createJointMotors(class btMultiBody* body);
	
	
	void	releaseSharedMemory();
	
	bool loadUrdf(const char* fileName, const class btVector3& pos, const class btQuaternion& orn,
                             bool useMultiBody, bool useFixedBase);

public:
	PhysicsServerSharedMemory();
	virtual ~PhysicsServerSharedMemory();

	//todo: implement option to allocated shared memory from client 
	virtual bool connectSharedMemory(bool allowSharedMemoryInitialization, class btMultiBodyDynamicsWorld* dynamicsWorld, struct GUIHelperInterface* guiHelper);

	virtual void disconnectSharedMemory (bool deInitializeSharedMemory);

	virtual void processClientCommands();

	bool	supportsJointMotor(class btMultiBody* body, int linkIndex);

};


#endif //PHYSICS_SERVER_EXAMPLESHARED_MEMORY_H


