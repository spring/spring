#ifndef CONNECTION_H
#define CONNECTION_H


namespace netcode
{
const unsigned NETWORK_BUFFER_SIZE = 40000;

/**
@brief Base class for connecting to various recievers / senders
*/
class CConnection
{
public:
	CConnection();
	virtual ~CConnection();
	
	virtual int SendData(const unsigned char *data, const unsigned length)=0;
	virtual int GetData(unsigned char *buf, const unsigned length)=0;

	virtual void Flush()=0;
	virtual void Ping()=0;

	bool active;
	float lastSendTime;
	float lastReceiveTime;
	
// protected:
	unsigned dataSent, sentOverhead;
	unsigned dataRecv, recvOverhead;
	
	static const unsigned char NETMSG_HELLO;

};

} // namespace netcode


#endif
