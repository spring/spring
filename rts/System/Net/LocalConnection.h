#ifndef _LOCALCONNECTION 
#define _LOCALCONNECTION

#include <boost/thread/mutex.hpp>

#include "Connection.h"
#include "Net.h"

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
	@return The amount of data sent (will be length)
	@throw network_error When data doesn't fit in the buffer
	*/
	virtual int SendData(const unsigned char *data, const unsigned length);
	
	/**
	@brief Get data
	@return The amount of data read
	@param length The maximum data length to copy
	@throw network_error When the data doesnt fit in *buf
	*/
	virtual int GetData(unsigned char *buf, const unsigned length);

	/// does nothing
	virtual void Flush(const bool forced = false);

private:
	static unsigned char Data[2][NETWORK_BUFFER_SIZE];
	static boost::mutex Mutex[2];
	static unsigned Length[2];

	unsigned OtherInstance() const;

	/// we can have 2 Instances, one in serverNet and one in net
	static unsigned Instances;
	/// which instance we are
	unsigned instance;
};

} // namespace netcode

#endif
