#include "Net.h"
#include <string.h>
#include <iostream>
#include <assert.h>

const unsigned int serverportnum = 5000;

bool TestLocal(netcode::CNet* server, unsigned freeNumber)
{
	std::cout << "Testing Local communication" << std::endl;
	netcode::CNet* localclient = new netcode::CNet();
	localclient->RegisterMessage((unsigned char)0, 10);
	localclient->RegisterMessage((unsigned char)1, 100);
	localclient->RegisterMessage((unsigned char)2, 1000);
	
	localclient->InitLocalClient(0);	
	server->InitLocalClient(freeNumber);
	
	server->Update();
	localclient->Update();
	
	unsigned char buffer[10];
	buffer[0] = 0;
	buffer[1] = 6;
	localclient->SendData(buffer, 10);
	localclient->FlushNet();
	server->Update();
	localclient->Update();
	unsigned char recvbuffer[4096];
	int ret = server->GetData(recvbuffer,0);
	if (ret != 10 || (int)recvbuffer[1] != 6)
	{
		std::cout << "Server dont recieved message" << std::endl;
		return false;
	}
	
	buffer[1] = 7;
	server->SendData(buffer, 10);
	ret = localclient->GetData(recvbuffer,0);
	if (ret != 10 || (int)recvbuffer[1] != 7)
	{
		std::cout << "Client dont recieved message" << std::endl;
		return false;
	}
	
	delete localclient;
	server->Kill(freeNumber);
	
	std::cout << "Test passed" << std::endl;
	return true;
}

int main()
{
	std::cout << "Starting Server and local Client" << std::endl;
	netcode::CNet* server = new netcode::CNet();
	server->InitServer(serverportnum);
	
	
	std::cout << "Registering messages..." << std::endl;
	server->RegisterMessage((unsigned char)0, 10);
	server->RegisterMessage((unsigned char)1, 100);
	server->RegisterMessage((unsigned char)2, 1000);
	
	if (!TestLocal(server, 0))
	{
		return 1;
	}
	
	std::cout << "Testing UDP networking..." << std::endl;
	
	netcode::CNet* client = new netcode::CNet();
	client->InitClient("localhost", serverportnum, 5001, 1);
	
	client->RegisterMessage((unsigned char)0, 10);
	client->RegisterMessage((unsigned char)1, 100);
	client->RegisterMessage((unsigned char)2, 1000);
	
	{
		unsigned char buffer[10];
		buffer[0] = 0;
		buffer[1] = 6;
		client->SendData(buffer, 10);
		client->Update();
		server->Update();
		server->AcceptIncomingConnection(1);
		server->Update();
		unsigned char recvbuffer[4096];
		int ret = server->GetData(recvbuffer,1);
		std::cout << "Recieved " << ret << " bytes UDP client message " << (int)recvbuffer[1] << std::endl;
		
		server->SendData(buffer, 10);
		sleep(1);	// buffer wont be flushed without this
		server->Update();
		client->Update();

		ret = client->GetData(recvbuffer,1);
		std::cout << "Recieved " << ret << " bytes UDP server message " << (int)recvbuffer[1] << std::endl;
		
		unsigned char bigbuffer[1000];
		bigbuffer[0]=2;
		client->SendData(bigbuffer, 1000);
		server->Update();
		client->Update();
		server->Update();
		ret = server->GetData(recvbuffer, 1);
		std::cout << "Recieved " << ret << " bytes UDP client message" << std::endl;
	}
	
	// 1 means true, 0 means false
	std::cout << "Server: Connected() = " << server->Connected() << std::endl;
	std::cout << "Client: Connected() = " << client->Connected() << std::endl;
	
	std::cout << "Deleting server and client..." << std::endl;
	delete server;
	delete client;
	
	return 0;
}
