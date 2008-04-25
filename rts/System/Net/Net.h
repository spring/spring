#ifndef NET_H
#define NET_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "RawPacket.h"

using netcode::RawPacket;

namespace netcode {
class CConnection;
struct NetAddress;
class UDPListener;

/**
@brief Interface for low level networking
Low level network connection (basically a fast TCP-like layer on top of UDP)
*/
class CNet
{
public:
	/**
	@brief Initialise the networking layer
	
	Only sets maximum mtu to 500.
	*/
	CNet();
	
	/**
	@brief Send all remaining data and exit.
	*/
	~CNet();
	
	/**
	@brief Listen for incoming connections
	
	Creates an UDPListener to listen for clients which want to connect.
	*/
	void InitServer(unsigned portnum);
	
	/**
	@brief Initialise Client
	@param server Address of the server, either IP or hostname
	@param portnum The port we have to connect to
	@param sourceport The port we will use here
	@param playernum The number this connection should get
	@return The number the new connection was assigned to (will be playerNum if this number is free, otherwise it will pick the first free number, starting from 0)
	
	This will spawn a new connection. Only do this when you cannot use a local connection, because they are somewhat faster.
	*/
	unsigned InitClient(const char* server,unsigned portnum,unsigned sourceport, unsigned playerNum);
	
	/** 
	@brief Init a local client
	@param wantedNumber The number this connection should get
	@return Like in InitClient, this will return the number thi connection was assigned to
	
	To increase performance, use this for local communication. You need to call ServerInitLocalClient() from inside the server too.
	*/
	unsigned InitLocalClient(const unsigned wantedNumber);
	
	/** 
	@brief Init a local client (call from inside the server)
	@todo This is only a workaround because we have no listener for local connections
	 */
	void ServerInitLocalClient();
	
	/**
	@brief register a new message type to the networking layer
	@param id Message identifier (has to be unique)
	@param length the length of the message (>0 if its fixed length, <0 means the next x bytes represent the length)
	
	Its not allowed to send unregistered messages. In this process you tell how big the messages are.
	*/
	void RegisterMessage(unsigned char id, int length);
	
	/**
	@brief Set maximum message size
	
	Default will be 500. Bigger messages will be truncated.
	*/
	void SetMTU(unsigned mtu = 500);
	
	/**
	@brief Check if new connections got accepted
	@return if new connections got accepted, it will return true. When its false, all packets from unknown sources will get dropped.
	*/
	bool Listening();
	
	/**
	@brief Set listening state
	@param state Wheter we accept packets from unknown sources (and create a new connection when we recieve one)
	*/
	void Listening(const bool state);
	
	/**
	@brief Kick a client
	@param connNumber client that should be kicked
	@throw network_error when there is no such connection
	
	Send all remaining data from buffer and then delete the connection.
	*/
	void Kill(const unsigned connNumber);

	/**
	@brief Are we already connected?
	@return true when we recieved data from someone
	
	This checks all connections if they recieved any data.
	*/
	bool Connected() const;

	/**
	@return The maximum connection number which is in use
	*/
	int MaxConnectionID() const;
	
	/**
	@brief Check if it is a valid connenction
	@return true when its valid, false when not
	@throw network_error When number is bigger then MaxConenctionID
	*/
	bool IsActiveConnection(const unsigned number) const;
	
	/**
	@brief Gives some usefull statistics
	@return string with statistics
	 */
	std::string GetConnectionStatistics(const unsigned number) const;
	NetAddress GetConnectedAddress(const unsigned number);

	/**
	@brief Take a look at the messages that will be returned by GetData().
	@return A RawPacket holding the data, or 0 if no data
	@param conNum The number to recieve from
	@param ahead How many packets to look ahead. A typical usage would be:
	for (int ahead = 0; (packet = net->Peek(conNum, ahead)) != NULL; ++ahead) {}
	*/
	const RawPacket* Peek(const unsigned conNum, unsigned ahead) const;

	/**
	@brief Recieve data from a client
	@param conNum The number to recieve from
	@return a RawPacket* with the data inside (or 0 when there is no data) (YOU! need to delete it after using)
	@throw network_error When conNum is not a valid connection ID
	*/
	RawPacket* GetData(const unsigned conNum);
	
	/**
	@brief Broadcast data to all clients
	@param data The data
	@param length length of the data
	@throw network_error Only when DEBUG is set: When the message identifier (data[0]) is not registered (through RegisterMessage())
	*/
	void SendData(const unsigned char* data,const unsigned length);
	void SendData(const RawPacket* data);
	
	/**
	@brief Send data to one client in particular
	@throw network_error When playerNum is no valid connection ID
	*/
	void SendData(const unsigned char* data,const unsigned length, const unsigned playerNum);
	
	/**
	@brief Send data to one client in particular
	@param data the RawPacket I will take care of, DO NOT DELETE IT
	@throw network_error When playerNum is no valid connection ID
	@todo there are too much SendData functions here
	 */
	void SendData(const RawPacket* data, const unsigned playerNum);
	
	/**
	@brief send all waiting data
	*/
	void FlushNet();
	
	/** 
	@brief Do this from time to time
	
	Updates the UDPlistener to recieve data from UDP and check for new connections. It also removes connections which are timed out.
	*/
	void Update();
	
	/// did someone tried to connect?
	bool HasIncomingConnection() const;
	
	/// Recieve data from first unbound connection to check if we allow him in our game
	RawPacket* GetData();
	
	/// everything seems fine, accept him
	unsigned AcceptIncomingConnection(const unsigned wantedNumber=0);
	
	/// we dont want you in our game
	void RejectIncomingConnection();
	
protected:
	/** Send a net message without any parameters. */
	void SendData(const unsigned char msg) {
		SendData(&msg, sizeof(msg));
	}

	/** Send a net message with one parameter. */
	template<typename A>
	void SendData(const unsigned char msg, const A a) {
		const int size = 1 + sizeof(A);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		SendData(buf, size);
	}

	template<typename A, typename B>
	void SendData(const unsigned char msg, const A a, const B b) {
		const int size = 1 + sizeof(A) + sizeof(B);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		SendData(buf, size);
	}

	template<typename A, typename B, typename C>
	void SendData(const unsigned char msg, const A a, const B b, const C c) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D>
	void SendData(const unsigned char msg, const A a, const B b, const C c, const D d) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D, typename E>
	void SendData(const unsigned char msg, A a, B b, C c, D d, E e) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		*(E*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D)] = e;
		SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D, typename E, typename F>
	void SendData(const unsigned char msg, A a, B b, C c, D d, E e, F f) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E) + sizeof(F);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		*(E*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D)] = e;
		*(F*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E)] = f;
		SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G>
	void SendData(const unsigned char msg, A a, B b, C c, D d, E e, F f, G g) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E) + sizeof(F) + sizeof(G);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		*(E*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D)] = e;
		*(F*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E)] = f;
		*(G*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E) + sizeof(F)] = g;
		SendData(buf, size);
	}
	
private:
	typedef boost::shared_ptr<CConnection> connPtr;
	typedef std::vector< connPtr > connVec;
	
	/**
	@brief Insert your Connection here to become connected
	@param newClient Connection to be inserted in the array
	@param wantedNumber 
	*/
	unsigned InitNewConn(const connPtr& newClient, const unsigned wantedNumber=0);
	
	/**
	@brief Holds the UDPListener for networking
	@todo make it more generic to allow for different protocols like TCP
	*/
	boost::scoped_ptr<UDPListener> udplistener;
	
	connPtr localConnBuf;
	
	/**
	@brief All active connections
	*/
	connVec connections;
};

} // namespace netcode

#endif /* NET_H */
