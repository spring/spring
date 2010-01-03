#ifdef _MSC_VER
#include "lib/streflop/streflop_cond.h"
#endif

#include "Connection.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "md5/md5.h"
#include "md5/base64.h"

#if BOOST_VERSION < 103600
using namespace boost::system::posix_error;
#else
using namespace boost::system::errc;
#endif
using namespace boost::asio;
boost::asio::io_service netservice;

Connection::Connection() : sock(netservice)
{
}

Connection::~Connection()
{
	sock.close();
	netservice.poll();
	netservice.reset();
}

void Connection::Connect(const std::string& server, int port)
{
	using namespace boost::asio;
	boost::system::error_code err;
	ip::address tempAddr = ip::address::from_string(server, err);
	if (err)
	{
		// error, maybe a hostname?
		ip::tcp::resolver resolver(netservice);
		std::ostringstream portbuf;
		portbuf << port;
		ip::tcp::resolver::query query(server, portbuf.str());
		ip::tcp::resolver::iterator iter = resolver.resolve(query, err);
		if (err)
		{
			DoneConnecting(false, err.message());
			return;
		}
		tempAddr = iter->endpoint().address();
	}

	ip::tcp::endpoint serverep(tempAddr, port);
	sock.async_connect(serverep, boost::bind(&Connection::ConnectCallback, this, placeholders::error));
}

std::string MD5Base64(const std::string& plain)
{
	md5_state_t state;
	md5_init(&state);
	md5_append(&state, (md5_byte_t*)plain.c_str(), plain.size());
	unsigned char digest[16];
	md5_finish(&state, digest);
	return base64_encode(digest, 16);
}

void Connection::Register(const std::string& name, const std::string& password)
{
	std::ostringstream out;
	out << "REGISTER " << name << " " << MD5Base64(password) << "\n";
	SendData(out.str());
}

void Connection::Login(const std::string& name, const std::string& password)
{
	std::ostringstream out;
	out << "LOGIN " << name << " " << MD5Base64(password) << " 0 localhost SLobby v0.1\n";
	SendData(out.str());
}

void Connection::SendData(const std::string& msg)
{
	sock.send(boost::asio::buffer(msg));
}

void Connection::Poll()
{
	netservice.poll();
}

void Connection::Run()
{
	netservice.run();
	netservice.reset();
}

void Connection::DataReceived(const std::string& command, const std::string& msg)
{
	if (command == "TASServer")
	{
		if (!msg.empty())
		{
			RawTextMessage buf(msg);
			std::string serverVer = buf.GetWord();
			std::string clientVer = buf.GetWord();
			int udpport = buf.GetInt();
			int mode = buf.GetInt();
			ServerGreeting(serverVer, clientVer, udpport, mode);
		}
	}
	else if (command == "DENIED")
	{
		Denied(msg);
	}
	else if (command == "REGISTRATIONDENIED")
	{
		RegisterDenied(msg);
	}
	else if (command == "REGISTRATIONACCEPTED")
	{
		RegisterAccept();
	}
	else
	{
		std::cout << "Unhandled command: " << command << " " << msg << std::endl;
	}
}

void Connection::ConnectCallback(const boost::system::error_code& error)
{
	if (!error)
	{
		DoneConnecting(true, "");
		boost::asio::async_read_until(sock, incomeBuffer, "\n", boost::bind(&Connection::ReceiveCallback, this, placeholders::error, placeholders::bytes_transferred));
	}
	else
	{
		DoneConnecting(false, error.message());
	}
}

void Connection::ReceiveCallback(const boost::system::error_code& error, size_t bytes)
{
	if (!error)
	{
		std::string msg;
		std::string command;
		std::istream buf(&incomeBuffer);
		buf >> command;
		std::getline(buf, msg);
		if (!msg.empty())
			msg = msg.substr(1);
		DataReceived(command, msg);
	}
	else
	{
		if (error.value() == connection_reset || error.value() == boost::asio::error::eof)
		{
			sock.close();
			Disconnected();
		}
		else
		{
			NetworkError(error.message());
		}
	}
	if (sock.is_open())
	{
		boost::asio::async_read_until(sock, incomeBuffer, "\n", boost::bind(&Connection::ReceiveCallback, this, placeholders::error, placeholders::bytes_transferred));
	}
}

void Connection::SendCallback(const boost::system::error_code& error)
{
	if (!error)
	{
	}
	else
	{
	}
}
