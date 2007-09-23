#ifndef CONNECTION_H
#define CONNECTION_H


namespace netcode
{
/**
@brief Base class for connecting to various recievers / senders
*/
class CConnection
{
public:
	CConnection();
	virtual ~CConnection();
	
	virtual void SendData(const unsigned char *data, const unsigned length)=0;
	virtual unsigned GetData(unsigned char *buf)=0;

	virtual void Flush(const bool forced = false)=0;
	virtual bool CheckTimeout() const = 0;
	
	unsigned GetDataRecieved() const;

protected:
	unsigned dataSent;
	unsigned dataRecv;
};

} // namespace netcode


#endif
