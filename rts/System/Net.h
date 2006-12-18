#ifndef NET_H
#define NET_H
// Net.h: interface for the CNet class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include "Platform/Win/win32.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <iostream>
#include <fstream>
#include <deque>
#include <map>
#include <algorithm>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_array.hpp>

class CFileHandler;

/*
Comment behind NETMSG enumeration constant gives the extra data belonging to
the net message. An empty comment means no extra data (message is only 1 byte).
Messages either consist of:
 1. uchar command; (NETMSG_* constant) and the specified extra data; or
 2. uchar command; uchar messageSize; and the specified extra data, for messages
    that contain a trailing std::string in the extra data; or
 3. uchar command; short messageSize; and the specified extra data, for messages
    that contain a trailing std::vector in the extra data.
Note that NETMSG_MAPDRAW can behave like 1. or 2. depending on the
CInMapDraw::NET_* command. messageSize is always the size of the entire message
including `command' and `messageSize'.
*/

enum NETMSG {
	NETMSG_HELLO            = 1,  //
	NETMSG_QUIT             = 2,  //
	NETMSG_NEWFRAME         = 3,  // int frameNum;
	NETMSG_STARTPLAYING     = 4,  //
	NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;
	NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;
	NETMSG_CHAT             = 7,  // uchar myPlayerNum; std::string message;
	NETMSG_RANDSEED         = 8,  // uint randSeed;
	NETMSG_COMMAND          = 11, // uchar myPlayerNum; int id; uchar options; std::vector<float> params;
	NETMSG_SELECT           = 12, // uchar myPlayerNum; std::vector<short> selectedUnitIDs;
	NETMSG_PAUSE            = 13, // uchar myPlayerNum, bNotPaused;
	NETMSG_AICOMMAND        = 14, // uchar myPlayerNum; short unitID; int id; uchar options; std::vector<float> params;
	NETMSG_SCRIPT           = 16, // std::string scriptName;
	NETMSG_MEMDUMP          = 17, // (NEVER SENT)
	NETMSG_MAPNAME          = 18, // uint checksum; std::string mapName;
	NETMSG_USER_SPEED       = 19, // float userSpeed;
	NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;
	NETMSG_CPU_USAGE        = 21, // float cpuUsage;
	NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;
	NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;
	NETMSG_ATTEMPTCONNECT   = 25, // uchar myPlayerNum, networkVersion;
	NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;
	NETMSG_SETSHARE         = 27, // uchar myTeam; float metalShareFraction, energyShareFraction;
	NETMSG_SENDPLAYERSTAT   = 28, //
	NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;
	NETMSG_GAMEOVER         = 30, //
	NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
	                              // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
	                              // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;
	NETMSG_SYNCREQUEST      = 32, // int frameNum;
	NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; CChecksum checksum;
	NETMSG_SYNCERROR        = 34, // (NEVER SENT)
	NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum; std::string message;
	NETMSG_STARTPOS         = 36, // uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;
	NETMSG_EXECHECKSUM      = 37, // uint checksum = 0;
	NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;
	NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left*/;
	NETMSG_MODNAME          = 40, // uint checksum; std::string modName;
#ifdef SYNCDEBUG
	NETMSG_SD_CHKREQUEST    = 41,
	NETMSG_SD_CHKRESPONSE   = 42,
	NETMSG_SD_BLKREQUEST    = 43,
	NETMSG_SD_BLKRESPONSE   = 44,
	NETMSG_SD_RESET         = 45,
#endif
};


#define NETWORK_BUFFER_SIZE 40000

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

class CNet
{
  private:
    struct AssembleBuffer 
    {
      boost::scoped_array<unsigned char> message_buffer;
      size_t index;
      AssembleBuffer( NETMSG msg, size_t buffer_size )
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
public:

	/** Send a net message without any parameters. */
	int SendData(NETMSG msg) {
		unsigned char t = msg;
		return SendData(&t, 1);
	}

	/** Send a net message with one parameter. */
	template<typename A>
	int SendData(NETMSG msg, A a) {
		const int size = 1 + sizeof(A);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		return SendData(buf, size);
	}

	template<typename A, typename B>
	int SendData(NETMSG msg, A a, B b) {
		const int size = 1 + sizeof(A) + sizeof(B);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C>
	int SendData(NETMSG msg, A a, B b, C c) {
		const int size = 1 + sizeof(A) + sizeof(B) + sizeof(C);
		unsigned char buf[size];
		buf[0] = msg;
		*(A*)&buf[1] = a;
		*(B*)&buf[1 + sizeof(A)] = b;
		*(C*)&buf[1 + sizeof(A) + sizeof(B)] = c;
		return SendData(buf, size);
	}

	template<typename A, typename B, typename C, typename D>
	int SendData(NETMSG msg, A a, B b, C c, D d) {
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
	int SendData(NETMSG msg, A a, B b, C c, D d, E e) {
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
	int SendData(NETMSG msg, A a, B b, C c, D d, E e, F f) {
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
	int SendData(NETMSG msg, A a, B b, C c, D d, E e, F f, G g) {
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
	int SendSTLData(NETMSG msg, const T& s) {
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
	int SendSTLData(NETMSG msg, A a, const T& s) {
		typedef typename T::value_type value_type;
		typedef typename is_string<T>::size_type size_type;

    size_type size =  1 + sizeof(size_type) + sizeof(A) + (s.size() + is_string<T>::TrailingNull) * sizeof(value_type);
    AssembleBuffer buf( msg, size );

    buf.add_scalar(size).add_scalar(a).add_sequence(s);

		return SendData(buf.get(), size);
	}

	template<typename A, typename B, typename T>
	int SendSTLData(NETMSG msg, A a, B b, const T& s) {
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
	int SendSTLData(NETMSG msg, A a, B b, C c, const T& s) {
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
	int SendSTLData(NETMSG msg, A a, B b, C c, D d, const T& s) {
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

	CNet();
	void StopListening();
	int GetData(unsigned char* buf,int length,int conNum);
	int SendData(unsigned char* data,int length);
	int SendData(unsigned char* data,int length,int connection);
	int InitClient(const char* server,int portnum,int sourceport,bool localConnect=false);
	int InitServer(int portnum);
	void Update(void);
	virtual ~CNet();

	bool connected;
	bool inInitialConnect;
	bool waitOnCon;

	bool imServer;
	bool onlyLocal;

	float curTime;

	struct Packet {
		int length;
		unsigned char* data;

		Packet(): data(0),length(0){};
		Packet(const void* indata,int length): length(length){data=SAFE_NEW unsigned char[length];memcpy(data,indata,length);};

		~Packet(){delete[] data;};
	};

	struct Connection {
		sockaddr_in addr;
		bool active;
		float lastReceiveTime;
		float lastSendTime;
		int lastSendFrame;
		Connection* localConnection;

		//outgoing stuff
		unsigned char outgoingData[NETWORK_BUFFER_SIZE];
		int outgoingLength;

		std::deque<Packet*> unackedPackets;
		int firstUnacked;
		int currentNum;

		//incomming stuff
		unsigned char readyData[NETWORK_BUFFER_SIZE];
		int readyLength;

		std::map<int,Packet*> waitingPackets;
		int lastInOrder;
		int lastNak;
		float lastNakTime;
	};
	Connection connections[MAX_PLAYERS];

#ifdef _WIN32
	SOCKET mySocket;
#else
	int mySocket;
#endif
	int InitNewConn(sockaddr_in* other,bool localConnect,int wantedNumber);
	int ResolveConnection(sockaddr_in* from);

	void ProcessRawPacket(unsigned char* data, int length, int conn);	
	void FlushNet(void);
	void FlushConnection(int conn);
	void SendRawPacket(int conn, unsigned char* data, int length, int packetNum);

	void CreateDemoFile();
	void SaveToDemo(unsigned char* buf,int length);
	bool FindDemoFile(const char* name);

	std::string demoName;
	std::ofstream* recordDemo;
	CFileHandler* playbackDemo;

	float demoTimeOffset;
	float nextDemoRead;

	unsigned char tempbuf[NETWORK_BUFFER_SIZE];

	void ReadDemoFile(void);
	void CreateDemoServer(std::string demoname);
	void StartDemoServer(void);

	// used by CPreGame to set init-data.
	void SetScript(const std::string& name);
	void SetMap(unsigned checksum, const std::string& name);
	void SetMod(unsigned checksum, const std::string& name);

private:

	// init-data.  These are send over when a new connection is initialized.
	std::string scriptName;
	unsigned mapChecksum;
	std::string mapName;
	unsigned modChecksum;
	std::string modName;
};

extern CNet* serverNet;
extern CNet* net;

#endif /* NET_H */
