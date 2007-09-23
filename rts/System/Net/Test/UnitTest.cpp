#include "Net.h"
#include <string.h>
#include <iostream>
#include <assert.h>

const unsigned int serverportnum = 5000;

int main()
{
	std::cout << "Starting Server and local Client" << std::endl;
	netcode::CNet* server = new netcode::CNet();
	server->InitServer(serverportnum);
	server->InitLocalClient(0);
	
	netcode::CNet* localclient = new netcode::CNet();
	localclient->InitLocalClient(0);
	
	server->Update();
	localclient->Update();
	
	std::cout << "Registering messages..." << std::endl;
	server->RegisterMessage((unsigned char)0, 10);
	server->RegisterMessage((unsigned char)1, 100);
	server->RegisterMessage((unsigned char)2, 1000);
	
	localclient->RegisterMessage((unsigned char)0, 10);
	localclient->RegisterMessage((unsigned char)1, 100);
	localclient->RegisterMessage((unsigned char)2, 1000);
	
	{
		unsigned char buffer[10];
		buffer[0] = 0;
		buffer[1] = 6;
		localclient->SendData(buffer, 10);
		server->Update();
		localclient->Update();
		unsigned char recvbuffer[4096];
		int ret = server->GetData(recvbuffer,0);
		std::cout << "Recieved " << ret << " bytes local client message " << (int)recvbuffer[1] << std::endl;
		
		server->SendData(buffer, 10);
		ret = localclient->GetData(recvbuffer,0);
		std::cout << "Recieved " << ret << " bytes local server message " << (int)recvbuffer[1] << std::endl;
		
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
		server->Update();
		client->Update();
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
	std::cout << "Local Client: Connected() = " << localclient->Connected() << std::endl;
	
	std::cout << "Deleting server and client..." << std::endl;
	delete server;
	delete localclient;
	delete client;
	
	return 0;
}
