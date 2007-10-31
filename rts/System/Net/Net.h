#ifndef NET_H
#define NET_H

#include <string>
#include <queue>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>


namespace netcode {

class CConnection;
class UDPListener;
class CLocalConnection;
class ProtocolDef;

const unsigned NETWORK_BUFFER_SIZE = 40000;
const unsigned MAX_CONNECTIONS = 32;

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

/**
@brief Interface for low level networking
Low level network connection (basically a fast TCP-like layer on top of UDP)
*/
class CNet
{
public:
	CNet();
	~CNet();
	
	/**
	@brief Initialise server
	@throw network_error If portnum not valid, used or other errors
	*/
	void InitServer(unsigned portnum);
	
	/**
	@brief Initialise Client
	@return The number the new connection was assigned to
	Only do this when you cannot use a local connection, 'cuz they are somewhat faster.
	*/
	unsigned InitClient(const char* server,unsigned portnum,unsigned sourceport, unsigned playerNum);
	
	/** 
	@brief Init a local client
	@return The number the new connection was assigned to
	To increase performance, use this for local communication. This can be called in server and in client mode
	*/
	unsigned InitLocalClient(const unsigned wantedNumber);
	
	/**
	@brief register a new message type to the networking layer
	Its not allowed to send unregistered messages. In this process you tell how big the messages are.
	@param id Message identifier (has to be unique)
	@param length the length of the message (>0 if its fixed length, <0 means the next x bytes represent the length)
	*/
	void RegisterMessage(unsigned char id, int length);
	
	/**
	@brief Set maximum message size
	*/
	unsigned SetMTU(unsigned mtu = 500);
	
	/// Are we accepting new connections?
	bool Listening();
	
	/// Set Listening state
	void Listening(const bool state);
	
	/**
	@brief Kill a client
	Forced flush and deactivate (delete) a connection
	*/
	void Kill(const unsigned connNumber);

	/// return true when local connected or already recieved data from remote
	bool Connected() const;

	/**
	@brief Check if it is a valid connenction
	@return true when its valid, false when not
	*/
	bool IsActiveConnection(const unsigned number) const;
	
	/**
	@brief Recieve data from a client
	*/
	int GetData(unsigned char* buf, const unsigned conNum);
	
	/** Broadcast data to all clients
	*/
	void SendData(const unsigned char* data,const unsigned length);
	
	/** Send data to one client in particular
	*/
	void SendData(const unsigned char* data,const unsigned length, const unsigned playerNum);
	
	/**
	@brief send all waiting data
	*/
	void FlushNet();
	
	/** 
	@brief Do this from time to time
	Updates the UDPlistener (you cant recieve data without doing this)
	*/
	void Update();
	
	/// did someone tried to connect?
	bool HasIncomingConnection() const;
	
	/// Recieve data from first unbound connection to check if we allow him in our game
	unsigned GetData(unsigned char* buf);
	
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

	/** Send a net message without any fixed size parameter but with a variable sized
	STL container parameter (e.g. std::string or std::vector). */
	template<typename T>
	void SendSTLData(const unsigned char msg, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size =  1 + sizeof(size_type) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );
		buf.add_scalar(size).add_sequence(s );

		SendData(buf.get(), size);
	}

	/** Send a net message with one fixed size parameter and a variable sized
	STL container parameter (e.g. std::string or std::vector). */
	template<typename A, typename T>
	void SendSTLData(const unsigned char msg, const A a, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size =  1 + sizeof(size_type) + sizeof(A) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );

		buf.add_scalar(size).add_scalar(a).add_sequence(s);

		SendData(buf.get(), size);
	}

	template<typename A, typename B, typename T>
	void SendSTLData(const unsigned char msg, const A a, const B b, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size = 1 + sizeof(size_type) + sizeof(A) + sizeof(B) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );

		buf.add_scalar(size)
				.add_scalar(a)
				.add_scalar(b)
				.add_sequence(s);

		SendData(buf.get(), size);
	}

	template<typename A, typename B, typename C, typename T>
	void SendSTLData(const unsigned char msg, A a, B b, C c, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

		size_type size = 1 + sizeof(size_type) + sizeof(A) + sizeof(B) + sizeof(C) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
		AssembleBuffer buf( msg, size );

		buf.add_scalar(size)
		.add_scalar(a)
		.add_scalar(b)
		.add_scalar(c)
		.add_sequence(s);

		SendData(buf.get(), size);
	}

	template<typename A, typename B, typename C, typename D, typename T>
	void SendSTLData(const unsigned char msg, A a, B b, C c, D d, const T& s) {
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

		SendData(buf.get(), size);
	}
	
private:
	/**
	@brief Insert your Connection here to become connected
	@param newClient Connection to be inserted in the array
	@param wantedNumber 
	*/
	unsigned InitNewConn(boost::shared_ptr<CConnection> newClient, const unsigned wantedNumber=0);

	const ProtocolDef* GetProto() const;
	
	/**
	@brief Holds the UDPListener for networking
	@todo make it more generic to allow for different protocols like TCP
	*/
	boost::scoped_ptr<UDPListener> udplistener;
	
	/**
	@brief All active connections
	@todo make it variable sized without performance penalty
	*/
	boost::shared_ptr<CConnection> connections[MAX_CONNECTIONS];
	std::queue< boost::shared_ptr<CConnection> > waitingQueue;
	ProtocolDef* proto;
	
	struct AssembleBuffer
	{
		boost::scoped_array<unsigned char> message_buffer;
		size_t index;
		AssembleBuffer( const unsigned char msg, size_t buffer_size )
			: message_buffer( new unsigned char[buffer_size] ), index(1)
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
	
	friend class UDPListener;
};

} // namespace netcode

#endif /* NET_H */
