#ifndef SHARED_MEMORY_BLOCK_H
#define SHARED_MEMORY_BLOCK_H

#define SHARED_MEMORY_KEY 12347
#define SHARED_MEMORY_MAGIC_NUMBER 64738
#define SHARED_MEMORY_MAX_COMMANDS 32
#define SHARED_MEMORY_MAX_STREAM_CHUNK_SIZE (256*1024)

#include "SharedMemoryCommands.h"

struct SharedMemoryBlock
{
	int m_magicId;
	struct SharedMemoryCommand m_clientCommands[SHARED_MEMORY_MAX_COMMANDS];
	struct SharedMemoryStatus m_serverCommands[SHARED_MEMORY_MAX_COMMANDS];

	int m_numClientCommands;
	int m_numProcessedClientCommands;

	int m_numServerCommands;
	int m_numProcessedServerCommands;

	//m_bulletStreamDataClientToServer is a way for the client to create collision shapes, rigid bodies and constraints
	//the Bullet data structures are more general purpose than the capabilities of a URDF file.
	char    m_bulletStreamDataClientToServer[SHARED_MEMORY_MAX_STREAM_CHUNK_SIZE];

	//m_bulletStreamDataServerToClient is used to send (debug) data from server to client, for
	//example to provide all details of a multibody including joint/link names, after loading a URDF file.
	char    m_bulletStreamDataServerToClient[SHARED_MEMORY_MAX_STREAM_CHUNK_SIZE];
};


static void     InitSharedMemoryBlock(struct SharedMemoryBlock* sharedMemoryBlock)
{
    sharedMemoryBlock->m_numClientCommands = 0;
    sharedMemoryBlock->m_numServerCommands = 0;
    sharedMemoryBlock->m_numProcessedClientCommands=0;
    sharedMemoryBlock->m_numProcessedServerCommands=0;
    sharedMemoryBlock->m_magicId = SHARED_MEMORY_MAGIC_NUMBER;
}

#define SHARED_MEMORY_SIZE sizeof(SharedMemoryBlock)




#endif //SHARED_MEMORY_BLOCK_H

