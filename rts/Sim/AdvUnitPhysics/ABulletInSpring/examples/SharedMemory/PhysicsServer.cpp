#include "PhysicsServer.h"
#include "PosixSharedMemory.h"
#include "Win32SharedMemory.h"

#include "../Importers/ImportURDFDemo/BulletUrdfImporter.h"
#include "../Importers/ImportURDFDemo/MyMultiBodyCreator.h"
#include "../Importers/ImportURDFDemo/URDF2Bullet.h"
#include "BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h"
#include "BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h"
#include "BulletDynamics/Featherstone/btMultiBodyPoint2Point.h"
#include "BulletDynamics/Featherstone/btMultiBodyLinkCollider.h"
#include "BulletDynamics/Featherstone/btMultiBodyJointFeedback.h"
#include "btBulletDynamicsCommon.h"

#include "../Extras/Serialize/BulletWorldImporter/btBulletWorldImporter.h"
#include "BulletDynamics/Featherstone/btMultiBodyJointMotor.h"
#include "LinearMath/btSerializer.h"
#include "Bullet3Common/b3Logging.h"
#include "../CommonInterfaces/CommonGUIHelperInterface.h"
#include "SharedMemoryBlock.h"

struct UrdfLinkNameMapUtil
{
	btMultiBody* m_mb;
	btDefaultSerializer* m_memSerializer;

	UrdfLinkNameMapUtil():m_mb(0),m_memSerializer(0)
	{
	}
};

struct PhysicsServerInternalData
{
	SharedMemoryInterface* m_sharedMemory;
    SharedMemoryBlock* m_testBlock1;
	bool m_isConnected;
	btScalar m_physicsDeltaTime;
	btAlignedObjectArray<btMultiBodyJointFeedback*> m_multiBodyJointFeedbacks;

	btAlignedObjectArray<btBulletWorldImporter*> m_worldImporters;
	btAlignedObjectArray<UrdfLinkNameMapUtil*> m_urdfLinkNameMapper;
	btHashMap<btHashInt, btMultiBodyJointMotor*>	m_multiBodyJointMotorMap;
	btAlignedObjectArray<std::string*> m_strings;

	btMultiBodyDynamicsWorld* m_dynamicsWorld;
	struct GUIHelperInterface* m_guiHelper;
	

	PhysicsServerInternalData()
		:m_sharedMemory(0),
		m_testBlock1(0),
		m_isConnected(false),
		m_physicsDeltaTime(1./60.),
		m_dynamicsWorld(0),
		m_guiHelper(0)
	{
	}

	SharedMemoryStatus& createServerStatus(int statusType, int sequenceNumber, int timeStamp)
	{
		SharedMemoryStatus& serverCmd =m_testBlock1->m_serverCommands[0];
		serverCmd .m_type = statusType;
		serverCmd.m_sequenceNumber = sequenceNumber;
		serverCmd.m_timeStamp = timeStamp;
		return serverCmd;
	}
	void submitServerStatus(SharedMemoryStatus& status)
	{
		m_testBlock1->m_numServerCommands++;
	}

};


PhysicsServerSharedMemory::PhysicsServerSharedMemory()
{
	m_data = new PhysicsServerInternalData();

#ifdef _WIN32
	m_data->m_sharedMemory = new Win32SharedMemoryServer();
#else
	m_data->m_sharedMemory = new PosixSharedMemory();
#endif


}

PhysicsServerSharedMemory::~PhysicsServerSharedMemory()
{
	delete m_data;
}



bool PhysicsServerSharedMemory::connectSharedMemory(bool allowSharedMemoryInitialization, class btMultiBodyDynamicsWorld* dynamicsWorld, struct GUIHelperInterface* guiHelper)
{
	m_data->m_dynamicsWorld = dynamicsWorld;
	m_data->m_guiHelper = guiHelper;
	
	bool allowCreation = true;
	bool allowConnectToExistingSharedMemory = false;

	m_data->m_testBlock1 = (SharedMemoryBlock*)m_data->m_sharedMemory->allocateSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE,allowCreation);
    if (m_data->m_testBlock1)
    {
        if (!allowConnectToExistingSharedMemory || (m_data->m_testBlock1->m_magicId !=SHARED_MEMORY_MAGIC_NUMBER))
        {
			if (allowSharedMemoryInitialization)
			{
				InitSharedMemoryBlock(m_data->m_testBlock1);
				b3Printf("Created and initialized shared memory block");
				m_data->m_isConnected = true;
			} else
			{
				b3Error("Error: please start server before client\n");
				m_data->m_sharedMemory->releaseSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE);
				m_data->m_testBlock1 = 0;
				return false;
			}
        } else
		{
			b3Printf("Connected to existing shared memory, status OK.\n");
			m_data->m_isConnected = true;
		}
    } else
	{
		b3Error("Cannot connect to shared memory");
		return false;
	}
	return true;
}


void PhysicsServerSharedMemory::disconnectSharedMemory(bool deInitializeSharedMemory)
{
	b3Printf("releaseSharedMemory1\n");
	if (m_data->m_testBlock1)
	{
		b3Printf("m_testBlock1\n");
		if (deInitializeSharedMemory)
		{
			m_data->m_testBlock1->m_magicId = 0;
			b3Printf("De-initialized shared memory, magic id = %d\n",m_data->m_testBlock1->m_magicId);
		}
		btAssert(m_data->m_sharedMemory);
		m_data->m_sharedMemory->releaseSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE);
	}
	if (m_data->m_sharedMemory)
	{
		b3Printf("m_sharedMemory\n");
		delete m_data->m_sharedMemory;
		m_data->m_sharedMemory = 0;
		m_data->m_testBlock1 = 0;
	}
}

void PhysicsServerSharedMemory::releaseSharedMemory()
{
	b3Printf("releaseSharedMemory1\n");
    if (m_data->m_testBlock1)
    {
		b3Printf("m_testBlock1\n");
        m_data->m_testBlock1->m_magicId = 0;
        b3Printf("magic id = %d\n",m_data->m_testBlock1->m_magicId);
        btAssert(m_data->m_sharedMemory);
        m_data->m_sharedMemory->releaseSharedMemory(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE);
    }
    if (m_data->m_sharedMemory)
    {
		b3Printf("m_sharedMemory\n");
        delete m_data->m_sharedMemory;
        m_data->m_sharedMemory = 0;
        m_data->m_testBlock1 = 0;
    }
}


bool PhysicsServerSharedMemory::supportsJointMotor(btMultiBody* mb, int mbLinkIndex)
{
	bool canHaveMotor = (mb->getLink(mbLinkIndex).m_jointType==btMultibodyLink::eRevolute
			||mb->getLink(mbLinkIndex).m_jointType==btMultibodyLink::ePrismatic);
	return canHaveMotor;

}

//for testing, create joint motors for revolute and prismatic joints
void	PhysicsServerSharedMemory::createJointMotors(btMultiBody* mb)
{
	int numLinks = mb->getNumLinks();
	for (int i=0;i<numLinks;i++)
	{
		int mbLinkIndex = i;
							
		if (supportsJointMotor(mb,mbLinkIndex))
		{
			float maxMotorImpulse = 0.f;
			int dof = 0;
			btScalar desiredVelocity = 0.f;
			btMultiBodyJointMotor* motor = new btMultiBodyJointMotor(mb,mbLinkIndex,dof,desiredVelocity,maxMotorImpulse);
			//motor->setMaxAppliedImpulse(0);
			m_data->m_multiBodyJointMotorMap.insert(mbLinkIndex,motor);
			m_data->m_dynamicsWorld->addMultiBodyConstraint(motor);
		}

	}
}



bool PhysicsServerSharedMemory::loadUrdf(const char* fileName, const btVector3& pos, const btQuaternion& orn,
                             bool useMultiBody, bool useFixedBase)
{
	btAssert(m_data->m_dynamicsWorld);
	if (!m_data->m_dynamicsWorld)
	{
		b3Error("loadUrdf: No valid m_dynamicsWorld");
		return false;
	}

    BulletURDFImporter u2b(m_data->m_guiHelper);

    bool loadOk =  u2b.loadURDF(fileName, useFixedBase);
    if (loadOk)
    {
        b3Printf("loaded %s OK!", fileName);
        
        btTransform tr;
        tr.setIdentity();
        tr.setOrigin(pos);
        tr.setRotation(orn);
        //int rootLinkIndex = u2b.getRootLinkIndex();
        //                      printf("urdf root link index = %d\n",rootLinkIndex);
		MyMultiBodyCreator creation(m_data->m_guiHelper);
        
        ConvertURDF2Bullet(u2b,creation, tr,m_data->m_dynamicsWorld,useMultiBody,u2b.getPathPrefix());
        btMultiBody* mb = creation.getBulletMultiBody();
		if (useMultiBody)
		{
			if (mb)
			{
				createJointMotors(mb);

				//serialize the btMultiBody and send the data to the client. This is one way to get the link/joint names across the (shared memory) wire
			    UrdfLinkNameMapUtil* util = new UrdfLinkNameMapUtil;
			    m_data->m_urdfLinkNameMapper.push_back(util);
			    util->m_mb = mb;
			    util->m_memSerializer = new btDefaultSerializer(SHARED_MEMORY_MAX_STREAM_CHUNK_SIZE,(unsigned char*)m_data->m_testBlock1->m_bulletStreamDataServerToClient);
			    //disable serialization of the collision objects (they are too big, and the client likely doesn't need them);
				util->m_memSerializer->m_skipPointers.insert(mb->getBaseCollider(),0);

			    for (int i=0;i<mb->getNumLinks();i++)
                {
					//disable serialization of the collision objects
                   util->m_memSerializer->m_skipPointers.insert(mb->getLink(i).m_collider,0);
				   int urdfLinkIndex = creation.m_mb2urdfLink[i];
				   std::string* linkName = new std::string(u2b.getLinkName(urdfLinkIndex).c_str());
				   m_data->m_strings.push_back(linkName);
				   util->m_memSerializer->registerNameForPointer(linkName->c_str(),linkName->c_str());
				   mb->getLink(i).m_linkName = linkName->c_str();

				   std::string* jointName = new std::string(u2b.getJointName(urdfLinkIndex).c_str());
				   m_data->m_strings.push_back(jointName);
				   util->m_memSerializer->registerNameForPointer(jointName->c_str(),jointName->c_str());
				   mb->getLink(i).m_jointName = jointName->c_str();
                }
				
				std::string* baseName = new std::string(u2b.getLinkName(u2b.getRootLinkIndex()));
				m_data->m_strings.push_back(baseName);

				
				util->m_memSerializer->registerNameForPointer(baseName->c_str(),baseName->c_str());
				mb->setBaseName(baseName->c_str());
				
				
                util->m_memSerializer->insertHeader();
                
                int len = mb->calculateSerializeBufferSize();
                btChunk* chunk = util->m_memSerializer->allocate(len,1);
                const char* structType = mb->serialize(chunk->m_oldPtr, util->m_memSerializer);
                util->m_memSerializer->finalizeChunk(chunk,structType,BT_MULTIBODY_CODE,mb);
                
				return true;
			} else
			{
				b3Warning("No multibody loaded from URDF. Could add btRigidBody+btTypedConstraint solution later.");
				return false;
			}
		} else
		{
			btAssert(0);
			
			return true;
		}
        
    }
    
    return false;
}







void PhysicsServerSharedMemory::processClientCommands()
{
	if (m_data->m_isConnected && m_data->m_testBlock1)
    {
        ///we ignore overflow of integer for now
        if (m_data->m_testBlock1->m_numClientCommands> m_data->m_testBlock1->m_numProcessedClientCommands)
        {
            
            //until we implement a proper ring buffer, we assume always maximum of 1 outstanding commands
            btAssert(m_data->m_testBlock1->m_numClientCommands==m_data->m_testBlock1->m_numProcessedClientCommands+1);
            
			const SharedMemoryCommand& clientCmd =m_data->m_testBlock1->m_clientCommands[0];
			m_data->m_testBlock1->m_numProcessedClientCommands++;
			//no timestamp yet
            int timeStamp = 0;

            //consume the command
			switch (clientCmd.m_type)
            {
				case CMD_SEND_BULLET_DATA_STREAM:
                {
					b3Printf("Processed CMD_SEND_BULLET_DATA_STREAM length %d",clientCmd.m_dataStreamArguments.m_streamChunkLength);
					
					btBulletWorldImporter* worldImporter = new btBulletWorldImporter(m_data->m_dynamicsWorld);
					m_data->m_worldImporters.push_back(worldImporter);
					bool completedOk = worldImporter->loadFileFromMemory(m_data->m_testBlock1->m_bulletStreamDataClientToServer,clientCmd.m_dataStreamArguments.m_streamChunkLength);
					
                    if (completedOk)
                    {
						SharedMemoryStatus& status = m_data->createServerStatus(CMD_BULLET_DATA_STREAM_RECEIVED_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
						m_data->m_guiHelper->autogenerateGraphicsObjects(this->m_data->m_dynamicsWorld);
						m_data->submitServerStatus(status);
                    } else
                    {
						SharedMemoryStatus& status = m_data->createServerStatus(CMD_BULLET_DATA_STREAM_RECEIVED_FAILED,clientCmd.m_sequenceNumber,timeStamp);
                        m_data->submitServerStatus(status);
                    }
                    
					break;
				}
                case CMD_LOAD_URDF:
                {
					//at the moment, we only load 1 urdf / robot
					if (m_data->m_urdfLinkNameMapper.size())
					{
						SharedMemoryStatus& status = m_data->createServerStatus(CMD_URDF_LOADING_FAILED,clientCmd.m_sequenceNumber,timeStamp);
						m_data->submitServerStatus(status);
						break;
					}
                    const UrdfArgs& urdfArgs = clientCmd.m_urdfArguments;
                    b3Printf("Processed CMD_LOAD_URDF:%s", urdfArgs.m_urdfFileName);
					btAssert((clientCmd.m_updateFlags&URDF_ARGS_FILE_NAME) !=0);
					btAssert(urdfArgs.m_urdfFileName);
					btVector3 initialPos(0,0,0);
					btQuaternion initialOrn(0,0,0,1);
					if (clientCmd.m_updateFlags & URDF_ARGS_INITIAL_POSITION)
					{
						initialPos[0] = urdfArgs.m_initialPosition[0];
						initialPos[1] = urdfArgs.m_initialPosition[1];
						initialPos[2] = urdfArgs.m_initialPosition[2];
					}
					if (clientCmd.m_updateFlags & URDF_ARGS_INITIAL_ORIENTATION)
					{
						initialOrn[0] = urdfArgs.m_initialOrientation[0];
						initialOrn[1] = urdfArgs.m_initialOrientation[1];
						initialOrn[2] = urdfArgs.m_initialOrientation[2];
						initialOrn[3] = urdfArgs.m_initialOrientation[3];
					}
					bool useMultiBody=(clientCmd.m_updateFlags & URDF_ARGS_USE_MULTIBODY) ? urdfArgs.m_useMultiBody : true;
					bool useFixedBase = (clientCmd.m_updateFlags & URDF_ARGS_USE_FIXED_BASE) ? urdfArgs.m_useFixedBase: false;

                    //load the actual URDF and send a report: completed or failed
                    bool completedOk = loadUrdf(urdfArgs.m_urdfFileName,
                                               initialPos,initialOrn,
                                               useMultiBody, useFixedBase);
                    SharedMemoryStatus& serverCmd =m_data->m_testBlock1->m_serverCommands[0];
 
                    if (completedOk)
                    {
						if (m_data->m_urdfLinkNameMapper.size())
						{
							serverCmd.m_dataStreamArguments.m_streamChunkLength = m_data->m_urdfLinkNameMapper.at(m_data->m_urdfLinkNameMapper.size()-1)->m_memSerializer->getCurrentBufferSize();
						}
						m_data->m_guiHelper->autogenerateGraphicsObjects(this->m_data->m_dynamicsWorld);
						SharedMemoryStatus& status = m_data->createServerStatus(CMD_URDF_LOADING_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
						m_data->submitServerStatus(status);
                        
                    } else
                    {
						SharedMemoryStatus& status = m_data->createServerStatus(CMD_URDF_LOADING_FAILED,clientCmd.m_sequenceNumber,timeStamp);
						m_data->submitServerStatus(status);
                    }
                    
                    

                    
                    break;
                }
                case CMD_CREATE_SENSOR:
                {
                    b3Printf("Processed CMD_CREATE_SENSOR");
                    
                    if (m_data->m_dynamicsWorld->getNumMultibodies()>0)
                    {
                        btMultiBody* mb = m_data->m_dynamicsWorld->getMultiBody(0);
                        btAssert(mb);
                        for (int i=0;i<clientCmd.m_createSensorArguments.m_numJointSensorChanges;i++)
                        {
                            int jointIndex = clientCmd.m_createSensorArguments.m_jointIndex[i];
                            if (clientCmd.m_createSensorArguments.m_enableJointForceSensor[i])
                            {
                               if (mb->getLink(jointIndex).m_jointFeedback)
                               {
                                   b3Warning("CMD_CREATE_SENSOR: sensor for joint [%d] already enabled", jointIndex);
                               } else
                               {
                                   btMultiBodyJointFeedback* fb = new btMultiBodyJointFeedback();
                                   fb->m_reactionForces.setZero();
                                   mb->getLink(jointIndex).m_jointFeedback = fb;
                                   m_data->m_multiBodyJointFeedbacks.push_back(fb);
                               };
                                
                            } else
                            {
                                if (mb->getLink(jointIndex).m_jointFeedback)
                                {
                                    m_data->m_multiBodyJointFeedbacks.remove(mb->getLink(jointIndex).m_jointFeedback);
                                    delete mb->getLink(jointIndex).m_jointFeedback;
                                    mb->getLink(jointIndex).m_jointFeedback=0;
                                } else
                                {
                                     b3Warning("CMD_CREATE_SENSOR: cannot perform sensor removal request, no sensor on joint [%d]", jointIndex);
                                };

                            }
                        }
                        
                    } else
                    {
                        b3Warning("No btMultiBody in the world. btRigidBody/btTypedConstraint sensor not hooked up yet");
                    }

#if 0
                    //todo(erwincoumans) here is some sample code to hook up a force/torque sensor for btTypedConstraint/btRigidBody
                    /*
                     for (int i=0;i<m_data->m_dynamicsWorld->getNumConstraints();i++)
                     {
                     btTypedConstraint* c = m_data->m_dynamicsWorld->getConstraint(i);
                     btJointFeedback* fb = new btJointFeedback();
                     m_data->m_jointFeedbacks.push_back(fb);
                     c->setJointFeedback(fb);
                     
                     
                     }
                     */
#endif
                   
                    SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_CLIENT_COMMAND_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
					m_data->submitServerStatus(serverCmd);
                    break;
                }
                case CMD_SEND_DESIRED_STATE:
                    {
                            b3Printf("Processed CMD_SEND_DESIRED_STATE");
                            if (m_data->m_dynamicsWorld->getNumMultibodies()>0)
                            {
                                btMultiBody* mb = m_data->m_dynamicsWorld->getMultiBody(0);
                                btAssert(mb);
                                
                                switch (clientCmd.m_sendDesiredStateCommandArgument.m_controlMode)
                                {
                                case CONTROL_MODE_TORQUE:
                                    {
										b3Printf("Using CONTROL_MODE_TORQUE");
                                        mb->clearForcesAndTorques();
                                        
                                        int torqueIndex = 0;
										btVector3 f(clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[0],
                                                    clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[1],
                                                    clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[2]);
                                        btVector3 t(clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[3],
                                                    clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[4],
                                                    clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[5]);
                                        torqueIndex+=6;
                                        mb->addBaseForce(f);
                                        mb->addBaseTorque(t);
                                        for (int link=0;link<mb->getNumLinks();link++)
                                        {
                                            
                                            for (int dof=0;dof<mb->getLink(link).m_dofCount;dof++)
                                            {               
                                                double torque = clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[torqueIndex];
                                                mb->addJointTorqueMultiDof(link,dof,torque);
                                                torqueIndex++;
                                            }
                                        }
                                        break;
                                    }
								case CONTROL_MODE_VELOCITY:
									{
                                        b3Printf("Using CONTROL_MODE_VELOCITY");
                                        
										int numMotors = 0;
										//find the joint motors and apply the desired velocity and maximum force/torque
										if (m_data->m_dynamicsWorld->getNumMultibodies()>0)
										{
											btMultiBody* mb = m_data->m_dynamicsWorld->getMultiBody(0);
											int dofIndex = 6;//skip the 3 linear + 3 angular degree of freedom entries of the base
											for (int link=0;link<mb->getNumLinks();link++)
											{
												if (supportsJointMotor(mb,link))
												{
													
													btMultiBodyJointMotor** motorPtr  = m_data->m_multiBodyJointMotorMap[link];
													if (motorPtr)
													{
														btMultiBodyJointMotor* motor = *motorPtr;
														btScalar desiredVelocity = clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateQdot[dofIndex];
														motor->setVelocityTarget(desiredVelocity);

														btScalar maxImp = clientCmd.m_sendDesiredStateCommandArgument.m_desiredStateForceTorque[dofIndex]*m_data->m_physicsDeltaTime;
														motor->setMaxAppliedImpulse(maxImp);
														numMotors++;

													}
												}
												dofIndex += mb->getLink(link).m_dofCount;
											}
										}
										break;
									}
                                default:
                                    {
                                        b3Printf("m_controlMode not implemented yet");
                                        break;
                                    }
                                    
                                }
                            }
                            
							SharedMemoryStatus& status = m_data->createServerStatus(CMD_DESIRED_STATE_RECEIVED_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
							m_data->submitServerStatus(status);
                        break;
                    }
				case CMD_REQUEST_ACTUAL_STATE:
					{
	                    b3Printf("Sending the actual state (Q,U)");
						if (m_data->m_dynamicsWorld->getNumMultibodies()>0)
						{
							btMultiBody* mb = m_data->m_dynamicsWorld->getMultiBody(0);
							SharedMemoryStatus& serverCmd = m_data->createServerStatus(CMD_ACTUAL_STATE_UPDATE_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);

							serverCmd.m_sendActualStateArgs.m_bodyUniqueId = 0;
							int totalDegreeOfFreedomQ = 0;
							int totalDegreeOfFreedomU = 0; 
							
							//always add the base, even for static (non-moving objects)
							//so that we can easily move the 'fixed' base when needed
							//do we don't use this conditional "if (!mb->hasFixedBase())"
							{
								btTransform tr;
								tr.setOrigin(mb->getBasePos());
								tr.setRotation(mb->getWorldToBaseRot().inverse());
								
								//base position in world space, carthesian
								serverCmd.m_sendActualStateArgs.m_actualStateQ[0] = tr.getOrigin()[0];
								serverCmd.m_sendActualStateArgs.m_actualStateQ[1] = tr.getOrigin()[1];
								serverCmd.m_sendActualStateArgs.m_actualStateQ[2] = tr.getOrigin()[2];

								//base orientation, quaternion x,y,z,w, in world space, carthesian
								serverCmd.m_sendActualStateArgs.m_actualStateQ[3] = tr.getRotation()[0]; 
								serverCmd.m_sendActualStateArgs.m_actualStateQ[4] = tr.getRotation()[1];
								serverCmd.m_sendActualStateArgs.m_actualStateQ[5] = tr.getRotation()[2];
								serverCmd.m_sendActualStateArgs.m_actualStateQ[6] = tr.getRotation()[3];
								totalDegreeOfFreedomQ +=7;//pos + quaternion

								//base linear velocity (in world space, carthesian)
								serverCmd.m_sendActualStateArgs.m_actualStateQdot[0] = mb->getBaseVel()[0];
								serverCmd.m_sendActualStateArgs.m_actualStateQdot[1] = mb->getBaseVel()[1];
								serverCmd.m_sendActualStateArgs.m_actualStateQdot[2] = mb->getBaseVel()[2];
	
								//base angular velocity (in world space, carthesian)
								serverCmd.m_sendActualStateArgs.m_actualStateQdot[3] = mb->getBaseOmega()[0];
								serverCmd.m_sendActualStateArgs.m_actualStateQdot[4] = mb->getBaseOmega()[1];
								serverCmd.m_sendActualStateArgs.m_actualStateQdot[5] = mb->getBaseOmega()[2];
								totalDegreeOfFreedomU += 6;//3 linear and 3 angular DOF
							}
							for (int l=0;l<mb->getNumLinks();l++)
							{
								for (int d=0;d<mb->getLink(l).m_posVarCount;d++)
								{
									serverCmd.m_sendActualStateArgs.m_actualStateQ[totalDegreeOfFreedomQ++] = mb->getJointPosMultiDof(l)[d];
								}
								for (int d=0;d<mb->getLink(l).m_dofCount;d++)
								{
									serverCmd.m_sendActualStateArgs.m_actualStateQdot[totalDegreeOfFreedomU++] = mb->getJointVelMultiDof(l)[d];
								}
                                
                                if (0 == mb->getLink(l).m_jointFeedback)
                                {
                                    for (int d=0;d<6;d++)
                                    {
                                        serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+d]=0;
                                    }
                                } else
                                {
                                    btVector3 sensedForce = mb->getLink(l).m_jointFeedback->m_reactionForces.getLinear();
                                    btVector3 sensedTorque = mb->getLink(l).m_jointFeedback->m_reactionForces.getLinear();
                                    
                                    serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+0] = sensedForce[0];
                                    serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+1] = sensedForce[1];
                                    serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+2] = sensedForce[2];
                                    
                                    serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+3] = sensedTorque[0];
                                    serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+4] = sensedTorque[1];
                                    serverCmd.m_sendActualStateArgs.m_jointReactionForces[l*6+5] = sensedTorque[2];
                                }
							}

							serverCmd.m_sendActualStateArgs.m_numDegreeOfFreedomQ = totalDegreeOfFreedomQ;
							serverCmd.m_sendActualStateArgs.m_numDegreeOfFreedomU = totalDegreeOfFreedomU;
							
							m_data->submitServerStatus(serverCmd);
							
						} else
						{

							b3Warning("Request state but no multibody available");
							SharedMemoryStatus& serverCmd = m_data->createServerStatus(CMD_ACTUAL_STATE_UPDATE_FAILED,clientCmd.m_sequenceNumber,timeStamp);
							m_data->submitServerStatus(serverCmd);
						}

						break;
					}
                case CMD_STEP_FORWARD_SIMULATION:
                {
                   
                    b3Printf("Step simulation request");
                    m_data->m_dynamicsWorld->stepSimulation(m_data->m_physicsDeltaTime);
                    
					SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_STEP_FORWARD_SIMULATION_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
					m_data->submitServerStatus(serverCmd);

                    break;
                }
					
				case CMD_SEND_PHYSICS_SIMULATION_PARAMETERS:
				{
					if (clientCmd.m_updateFlags&SIM_PARAM_UPDATE_GRAVITY)
					{
						btVector3 grav(clientCmd.m_physSimParamArgs.m_gravityAcceleration[0],
									   clientCmd.m_physSimParamArgs.m_gravityAcceleration[1],
									   clientCmd.m_physSimParamArgs.m_gravityAcceleration[2]);
						this->m_data->m_dynamicsWorld->setGravity(grav);
						b3Printf("Updated Gravity: %f,%f,%f",grav[0],grav[1],grav[2]);

					}
					
					
					SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_CLIENT_COMMAND_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
					m_data->submitServerStatus(serverCmd);
					break;
					
				};
				case CMD_INIT_POSE:
				{
					b3Printf("Server Init Pose not implemented yet");
					///@todo: implement this
					m_data->m_dynamicsWorld->setGravity(btVector3(0,0,0));
					
					SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_CLIENT_COMMAND_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
					m_data->submitServerStatus(serverCmd);

					break;
				}
					

					
                case CMD_SHUTDOWN:
                {
					btAssert(0);
                    //wantsShutdown = true;
					SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_CLIENT_COMMAND_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
					m_data->submitServerStatus(serverCmd);

                    break;
                }
				case CMD_CREATE_BOX_COLLISION_SHAPE:
					{
                        btVector3 halfExtents(1,1,1);
                        if (clientCmd.m_updateFlags | BOX_SHAPE_HAS_HALF_EXTENTS)
                        {
                            halfExtents = btVector3(
                                                  clientCmd.m_createBoxShapeArguments.m_halfExtentsX,
                                                  clientCmd.m_createBoxShapeArguments.m_halfExtentsY,
                                                  clientCmd.m_createBoxShapeArguments.m_halfExtentsZ);
                        }
						btTransform startTrans;
						startTrans.setIdentity();
                        if (clientCmd.m_updateFlags | BOX_SHAPE_HAS_INITIAL_POSITION)
                        {
                            startTrans.setOrigin(btVector3(
                                                 clientCmd.m_createBoxShapeArguments.m_initialPosition[0],
                                                 clientCmd.m_createBoxShapeArguments.m_initialPosition[1],
                                                 clientCmd.m_createBoxShapeArguments.m_initialPosition[2]));
                        }

                        if (clientCmd.m_updateFlags | BOX_SHAPE_HAS_INITIAL_ORIENTATION)
                        {
                            startTrans.setRotation(btQuaternion(
                                                           clientCmd.m_createBoxShapeArguments.m_initialOrientation[0],
                                                                 clientCmd.m_createBoxShapeArguments.m_initialOrientation[1],
                                                                 clientCmd.m_createBoxShapeArguments.m_initialOrientation[2],
                                                                 clientCmd.m_createBoxShapeArguments.m_initialOrientation[3]));
                        }

						btBulletWorldImporter* worldImporter = new btBulletWorldImporter(m_data->m_dynamicsWorld);
						m_data->m_worldImporters.push_back(worldImporter);

						btCollisionShape* shape = worldImporter->createBoxShape(halfExtents);
						btScalar mass = 0.f;
						bool isDynamic = (mass>0);
						worldImporter->createRigidBody(isDynamic,mass,startTrans,shape,0);
						m_data->m_guiHelper->autogenerateGraphicsObjects(this->m_data->m_dynamicsWorld);
						
						SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_CLIENT_COMMAND_COMPLETED,clientCmd.m_sequenceNumber,timeStamp);
						m_data->submitServerStatus(serverCmd);

						break;
					}
                default:
                {
                    b3Error("Unknown command encountered");
					
					SharedMemoryStatus& serverCmd =m_data->createServerStatus(CMD_UNKNOWN_COMMAND_FLUSHED,clientCmd.m_sequenceNumber,timeStamp);
					m_data->submitServerStatus(serverCmd);

                }
            };
            
        }
    }
}
