#include "LocalConnection.h"

#include <string.h>
#include <stdexcept>

namespace netcode {

// static stuff
unsigned CLocalConnection::Instances = 0;
unsigned char CLocalConnection::Data[2][NETWORK_BUFFER_SIZE];
boost::mutex CLocalConnection::Mutex[2];
unsigned CLocalConnection::Length[2] = {0,0};

CLocalConnection::CLocalConnection()
{
// TODO check and prevent further instances if we already have 2
	instance = Instances;
	Instances++;
}

CLocalConnection::~CLocalConnection()
{
	// logOutput.Print("Network statistics for local Connection");
	// logOutput.Print("Bytes send/recieved: %i/%i (Overhead: %i/%i)", dataSent, dataRecv, sentOverhead, recvOverhead);
	// logOutput.Print("Deleting %i bytes unhandled data", Length[instance]);
	Instances--; // not sure this is needed
}

int CLocalConnection::SendData(const unsigned char *data, const unsigned length)
{
	boost::mutex::scoped_lock scoped_lock(Mutex[OtherInstance()]);
	
	if(Length[OtherInstance()]+length>=NETWORK_BUFFER_SIZE){
		throw std::length_error("Buffer overflow in CLocalConnection::SendData");
	}
	memcpy(&Data[OtherInstance()][Length[OtherInstance()]],data,length);
	Length[OtherInstance()]+=length;
	dataSent += length;
	
	return length;
}

int CLocalConnection::GetData(unsigned char *buf, const unsigned length)
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
		
	unsigned ret=Length[instance];
	if(length<=ret) {
		throw std::length_error("Buffer overflow in CLocalConnection::GetData");
	}
	dataRecv += Length[instance];
	memcpy(buf,Data[instance],ret);
	Length[instance]=0;
	
	return ret;
}

void CLocalConnection::Flush(const bool forced)
{
}

unsigned CLocalConnection::OtherInstance() const
{
	if (instance == 0)
		return 1;
	else
		return 0;
}

} // namespace netcode

