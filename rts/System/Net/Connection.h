#ifndef CONNECTION_H
#define CONNECTION_H


namespace netcode
{
class RawPacket;

/**
@brief Base class for connecting to various recievers / senders
*/
class CConnection
{
public:
	CConnection();
	virtual ~CConnection();
	
	virtual void SendData(const unsigned char *data, const unsigned length)=0;

	/**
	@brief Take a look at the messages that will be returned by GetData().
	@return A RawPacket holding the data, or 0 if no data
	@param ahead How many packets to look ahead. A typical usage would be:
	for (int ahead = 0; (packet = conn->Peek(ahead)) != NULL; ++ahead) {}
	*/
	virtual const RawPacket* Peek(unsigned ahead) const = 0;
	
	/**
	@brief New method of data gathering
	@return A RawPacket holding the data, or 0 if no data
	*/
	virtual RawPacket* GetData()=0;

	virtual void Flush(const bool forced = false)=0;
	virtual bool CheckTimeout() const = 0;
	
	unsigned GetDataRecieved() const;

protected:
	unsigned dataSent;
	unsigned dataRecv;
};

} // namespace netcode


#endif
