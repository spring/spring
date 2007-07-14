#ifndef NET_H
#define NET_H
// Net.h: interface for the CNet class.
//
//////////////////////////////////////////////////////////////////////

// Network dependent includes
#ifdef _WIN32
#include "Platform/Win/win32.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

// general includes
#include <string>
#include <vector>
#include <boost/scoped_array.hpp>
#include <stdexcept>

// spring includes
#include "Connection.h"
#include "GlobalStuff.h"
#include "Sync/Syncify.h"

namespace netcode {

#ifndef _WIN32
	typedef int SOCKET;
#endif


/**
* network_error
* thrown when network error occured
*/
class network_error : public std::runtime_error
{
public:
	network_error(const std::string& msg) :
	std::runtime_error(msg) {}
};
	
// If we switch to a networking lib and start using a bitstream, we might
// as well remove this and use int as size type (because it'd be compressed anyway).
template<typename T> struct is_string    {
	typedef unsigned short size_type;
	enum { TrailingNull = 0 };
};
template<> struct is_string<std::string> {
	typedef unsigned char size_type;
	enum { TrailingNull = 1 };
};

/** Low level network connection (basically a fast TCP-like layer on top of UDP)
With also demo file writing and some other stuff hacked in. */
class CNet
{
public:
	CNet();
	~CNet();
	
	/** Stop the server from accepting new connections	
	*/
	void StopListening();
	/** Flush and deactivate a connection
	*/
	void Kill(const unsigned connNumber);
	/** Send an empty Packet to all Connections (NETMSG_HELLO)
	*/
	void PingAll();

	bool connected;
	bool inInitialConnect;
	bool waitOnCon;	// do we accept new clients

	bool imServer;
	bool onlyLocal;

	CConnection* connections[MAX_PLAYERS];
	int GetData(unsigned char* buf, const unsigned length, const unsigned conNum);
	
protected:
	int InitServer(unsigned portnum);
	int InitClient(const char* server,unsigned portnum,unsigned sourceport, unsigned playerNum);
	/** 
	@brief Init a client when a server runs in the same process
	To increase performance, use this shortcut to communicate with the server to rapidly increase performance
	*/
	int InitLocalClient(const unsigned wantedNumber);
	
	/** Struct to hold data of yet unaccepted clients
	*/
	struct Pending
	{
		sockaddr_in other;
		unsigned char networkVersion;
		unsigned char netmsg;
		unsigned char wantedNumber;
	};
	/** Insert your struct here to become connected
	*/
	int InitNewConn(const Pending& NewClient, bool local = false);	// don't call this directly when in server mode, use CNetProto instead
	
	/** Broadcast data to all clients
	*/
	int SendData(const unsigned char* data,const unsigned length);
	/** 
	@brief Do this from time to time
	1. Check our socket for incoming data and and push it to the according CConnection-class
	2. Push connection attempts in justConnected
	3. Update() each CConnection
	*/
	void Update(void);
	void FlushNet(void);
	
	/**
	@brief if new clients try to connect, their data is stored here until they get accepted / rejected
	This is where Update() stores the structs
	*/
	std::vector<Pending> justConnected;

	/** Send a net message without any parameters. */
	int SendData(const unsigned char msg) {
		return SendData(&msg, sizeof(msg));
	}

	/** Send a net message with one parameter. */
	template<typename A>
	int SendData(const unsigned char msg, const A a) {
		const int size = 1 + sizeof(A);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		return SendData(buf, size);
	}

	template<typename A, typename B>
	int SendData(const unsigned char msg, const A a, const B b) {
		const int size = 1 + sizeof(A) + sizeof(B);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C>
	int SendData(const unsigned char msg, const A a, const B b, const C c) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D>
	int SendData(const unsigned char msg, const A a, const B b, const C c, const D d) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D, typename E>
	int SendData(const unsigned char msg, A a, B b, C c, D d, E e) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		*(E*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D)] = e;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D, typename E, typename F>
	int SendData(const unsigned char msg, A a, B b, C c, D d, E e, F f) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E) + sizeof(F);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		*(D*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C)] = d;
		*(E*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D)] = e;
		*(F*)&buf[1 + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + sizeof(E)] = f;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G>
	int SendData(const unsigned char msg, A a, B b, C c, D d, E e, F f, G g) {
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
		return SendData(buf, size);
	}

	/** Send a net message without any fixed size parameter but with a variable sized
	STL container parameter (e.g. std::string or std::vector). */
	template<typename T>
	int SendSTLData(const unsigned char msg, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size =  1 + sizeof(size_type) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );
		buf.add_scalar(size).add_sequence(s );

		return SendData(buf.get(), size);
	}

	/** Send a net message with one fixed size parameter and a variable sized
	STL container parameter (e.g. std::string or std::vector). */
	template<typename A, typename T>
	int SendSTLData(const unsigned char msg, const A a, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size =  1 + sizeof(size_type) + sizeof(A) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );

		buf.add_scalar(size).add_scalar(a).add_sequence(s);

		return SendData(buf.get(), size);
	}

	template<typename A, typename B, typename T>
	int SendSTLData(const unsigned char msg, const A a, const B b, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size = 1 + sizeof(size_type) + sizeof(A) + sizeof(B) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );

		buf.add_scalar(size)
				.add_scalar(a)
				.add_scalar(b)
				.add_sequence(s);

		return SendData(buf.get(), size);
	}

	template<typename A, typename B, typename C, typename T>
	int SendSTLData(const unsigned char msg, A a, B b, C c, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size = 1 + sizeof(size_type) + sizeof(A) + sizeof(B) + sizeof(C) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );

		buf.add_scalar(size)
		.add_scalar(a)
		.add_scalar(b)
		.add_scalar(c)
		.add_sequence(s);

		return SendData(buf.get(), size);
	}

	template<typename A, typename B, typename C, typename D, typename T>
	int SendSTLData(const unsigned char msg, A a, B b, C c, D d, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size = 1 + sizeof(size_type) + sizeof(A) + sizeof(B) + sizeof(C) + sizeof(D) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);

		AssembleBuffer buf( msg, size );
		buf.add_scalar(size)
		.add_scalar(a)
		.add_scalar(b)
		.add_scalar(c)
		.add_scalar(d)
		.add_sequence(s);

		return SendData(buf.get(), size);
	}
	
private:
	/**
	@brief determine which connection has the specified address
	@return the number of the connection it matches, or -1 if it doesnt match any con
	*/
	int ResolveConnection(const sockaddr_in* from) const;
	SOCKET mySocket;
	
	struct AssembleBuffer
	{
		boost::scoped_array<unsigned char> message_buffer;
		size_t index;
		AssembleBuffer( const unsigned char msg, size_t buffer_size )
			: message_buffer( SAFE_NEW unsigned char[buffer_size] ), index(1)
		{ message_buffer[0] = msg; }

		template<typename T>
				AssembleBuffer& add_scalar( T const& obj)
		{
			* reinterpret_cast<T*>( message_buffer.get() + index) = obj;
			index += sizeof(T);
			return *this;
		}

		template<typename T>
				AssembleBuffer& add_sequence( T const& obj)
		{
			typedef typename T::value_type value_type;
			value_type * pos = reinterpret_cast<value_type*>( message_buffer.get() + index);
			std::copy( obj.begin(), obj.end(), pos );
			index += sizeof(value_type)*obj.size() + is_string<T>::TrailingNull;
			if( is_string<T>::TrailingNull ) {
				pos += obj.size();
				*pos = typename T::value_type(0);
			}
			return *this;
		}
		unsigned char * get() const { return message_buffer.get(); };
	};
};

} // namespace netcode

#endif /* NET_H */
