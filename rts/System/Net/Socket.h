#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <boost/noncopyable.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <netinet/in.h>
#endif

namespace netcode
{

enum SocketType { TCP, UDP };

/**
	@author Karl-Robert Ernst <k-r.ernst@my-mail.ch>
	@brief Base class for all Sockets
*/
class Socket : public boost::noncopyable
{
public:
	/**
	@brief Create Socket, initialise winsock when needed
	@param SocketType TCP or UDP?
	@throw network_error When an eror occurs
	*/
	Socket(const SocketType);
	
	/**
	@brief Close Socket, uninitialise winsock
	*/
	~Socket();
	
	/**
	@brief Resolves a host
	@param address The host's address, can be an IP-Address or an Hostname
	@param port the host's port
	@return the address-struct
	*/
	sockaddr_in ResolveHost(const std::string& address, const unsigned port) const;
	
protected:
	/// Set the blocking state of the socket
	void SetBlocking(const bool block) const;
	
	/// return the last errormessage from the OS
	std::string GetErrorMsg() const;
	/// Check if last error is a real error (not EWOULDBLOCK etc.)
	bool IsFakeError() const;
	
#ifdef _WIN32
	SOCKET mySocket;

	/**
	@brief Counts the amount of active sockets on the system
	
	The only use is to know wheter winsock needs to be initialised (when its the first socket) or uninitialised (when its the last).
	*/
	static unsigned numSockets;
#else
	int mySocket;
#endif
};

} // namespace netcode

#endif
