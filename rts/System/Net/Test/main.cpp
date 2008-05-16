#include "Net.h"
#include <string.h>
#include <iostream>
#include <assert.h>
#include <boost/shared_ptr.hpp>

const unsigned int serverportnum = 5000;
boost::shared_ptr<const netcode::RawPacket> data1;
boost::shared_ptr<const netcode::RawPacket> data2;

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
	
	
	localclient->SendData(data1);
	localclient->FlushNet();
	server->Update();
	localclient->Update();
	boost::shared_ptr<const netcode::RawPacket> ret = server->GetData(0);
	if (!ret || ret->length != 10 || (int)ret->data[1] != 6)
	{
		std::cout << "Server dont recieved message" << ret->length << std::endl;
		return false;
	}
		
	server->SendData(data1);
	ret = localclient->GetData(0);
	if (!ret || ret->length != 10 || (int)ret->data[1] != 6)
	{
		std::cout << "Client dont recieved message" << std::endl;
		return false;
	}
	
	std::cout << localclient->GetConnectionStatistics(0);
	delete localclient;
	server->Kill(freeNumber);
	
	std::cout << "Test passed" << std::endl;
	return true;
}

int main(int argc, const char* const* argv)
{
	// prepare test packets:
	unsigned char buffer[10];
	memset(buffer, 0, 10);
	buffer[1] = 6;
	data1.reset(new RawPacket(buffer, 10));
	
	unsigned char bigbuffer[1000];
	memset(bigbuffer, 0, 1000);
	bigbuffer[0]=2;
	data2.reset(new RawPacket(bigbuffer, 1000));
	
	std::cout << "Starting Server and local Client" << std::endl;
	netcode::CNet* server = new netcode::CNet();
	server->InitServer(serverportnum);
	
	
	std::cout << "Registering messages..." << std::endl;
	server->RegisterMessage((unsigned char)0, 10);
	server->RegisterMessage((unsigned char)1, 100);
	server->RegisterMessage((unsigned char)2, 1000);
	server->SendData(data1);
	
	if (!TestLocal(server, 0))
	{
		return 1;
	}
	
	std::cout << "Testing UDP networking..." << std::endl;
	
	netcode::CNet* client = new netcode::CNet();
	client->InitClient(argc > 1 ? argv[1] : "localhost", serverportnum, 5001, 1);
	
	client->RegisterMessage((unsigned char)0, 10);
	client->RegisterMessage((unsigned char)1, 100);
	client->RegisterMessage((unsigned char)2, 1000);
	
	{
		client->SendData(data1);
		client->Update();
		server->Update();
		server->AcceptIncomingConnection(1);
		server->Update();
		boost::shared_ptr<const netcode::RawPacket> ret  = server->GetData(1);
		if (ret)
		{
			std::cout << "Recieved " << ret->length << " bytes UDP client message " << (int)ret->data[1] << std::endl;
			ret.reset();
		}
		else
			return 1;
		
		server->SendData(data1);
		sleep(1);	// buffer wont be flushed without this
		server->Update();
		client->Update();

		ret = client->GetData(1);
		if (ret)
		{
			std::cout << "Recieved " << ret->length << " bytes UDP server message " << (int)ret->data[1] << std::endl;
			ret.reset();
		}
		else
			return 1;
		
		client->SendData(data2);
		server->Update();
		client->Update();
		server->Update();
		ret = server->GetData(1);
		if (ret)
		{
			std::cout << "Recieved " << ret->length << " bytes UDP client message" << std::endl;
			ret.reset();
		}
		else
			return 1;
	}
	
	// 1 means true, 0 means false
	std::cout << "Server: Connected() = " << server->Connected() << std::endl;
	std::cout << "Client: Connected() = " << client->Connected() << std::endl;
	
	std::cout << "Deleting server and client..." << std::endl;
	delete server;
	std::cout << client->GetConnectionStatistics(1);
	delete client;
	return 0;
}
