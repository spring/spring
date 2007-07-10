#ifndef _LOCALCONNECTION 
#define _LOCALCONNECTION

#include <boost/thread/mutex.hpp>

#include "Connection.h"

namespace netcode {

/**
@brief Class for local connection between server / client
Directly connects to each others inputbuffer to increase performance. The server and the client have to run in one instance of spring for this to work, otherwise a normal UDP connection had to be used.
IMPORTANT: You must not have more than two instances of this
 */
class CLocalConnection : public CConnection
{
public:
	CLocalConnection();
	virtual ~CLocalConnection();
	
	/**
	@brief Send data to other instance
	@return 0 on success, 1 on failure (buffer overflow)
	*/
	virtual int SendData(const unsigned char *data, const unsigned length);
	virtual int GetData(unsigned char *buf, const unsigned length);

	/// does nothing
	virtual void Update(const bool inInitialConnect);
	/// does nothing
	virtual void ProcessRawPacket(const unsigned char* data, const unsigned length);

	/// does nothing
	virtual void Flush();
	/// does nothing
	virtual void Ping();

	/// returns always false
	virtual bool CheckAddress(const sockaddr_in&) const;

private:
	static unsigned char Data[2][NETWORK_BUFFER_SIZE];
	static boost::mutex Mutex[2];
	static unsigned Length[2];

	unsigned otherInstance() const;

	/// we can have 2 Instances, one in serverNet and one in net
	static unsigned Instances;
	/// which instance we are
	unsigned instance;
};

} // namespace netcode

#endif
