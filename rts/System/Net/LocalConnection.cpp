#include "LocalConnection.h"

#include "LogOutput.h"

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
	if(active){
		if(Length[OtherInstance()]+length>=NETWORK_BUFFER_SIZE){
			logOutput.Print("Overflow when sending to local connection: Outbuf: %i Data: %i BuffSize: %i",Length[OtherInstance()],length,NETWORK_BUFFER_SIZE);
			return 0;
		}
		memcpy(&Data[OtherInstance()][Length[OtherInstance()]],data,length);
		Length[OtherInstance()]+=length;
		dataSent += length;
		// logOutput.Print("Sent %i bytes to %i", length, OtherInstance());
	}
	return 1;
}

int CLocalConnection::GetData(unsigned char *buf, const unsigned length)
{
	boost::mutex::scoped_lock scoped_lock(Mutex[instance]);
	if(active){
		unsigned ret=Length[instance];
		if(length<=ret) {
			logOutput.Print("Too small buffer to get data from local connection: Buffer: %i Datalength: %i",length, ret);
			return -1;
		}
		dataRecv += Length[instance];
		memcpy(buf,Data[instance],ret);
		Length[instance]=0;
		// logOutput.Print("%i recieved %i bytes", instance, ret);
		return ret;
	} else {
		logOutput.Print("Tried to get data from inactive connection");
		return -1;
	}
}

void CLocalConnection::Flush()
{
}

void CLocalConnection::Ping()
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

