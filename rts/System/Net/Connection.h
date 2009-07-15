#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <boost/shared_ptr.hpp>

#include "RawPacket.h"

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
	
	virtual void SendData(boost::shared_ptr<const RawPacket> data)=0;

	virtual bool HasIncomingData() const = 0;

	/**
	@brief Take a look at the messages that will be returned by GetData().
	@return A RawPacket holding the data, or 0 if no data
	@param ahead How many packets to look ahead. A typical usage would be:
	for (int ahead = 0; (packet = conn->Peek(ahead)); ++ahead) {}
	*/
	virtual boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const = 0;
	
	/**
	@brief New method of data gathering
	@return A RawPacket holding the data, or 0 if no data
	*/
	virtual boost::shared_ptr<const RawPacket> GetData()=0;

	virtual void Flush(const bool forced = false)=0;
	virtual bool CheckTimeout() const = 0;
	
	unsigned GetDataReceived() const;
	
	virtual std::string Statistics() const = 0;
	virtual std::string GetFullAddress() const = 0;
	virtual void Update() {};

protected:
	unsigned dataSent;
	unsigned dataRecv;
};

} // namespace netcode


#endif
