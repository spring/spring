#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "Connection.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#if BOOST_VERSION < 103600
using namespace boost::system::posix_error;
#else
using namespace boost::system::errc;
#endif

boost::asio::io_service netservice;


Connection::Connection() : sock(netservice)
{
}

Connection::~Connection()
{
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
		ip::tcp::resolver::iterator iter = resolver.resolve(query);
		tempAddr = iter->endpoint().address();
	}

	ip::tcp::endpoint serverep(tempAddr, port);
	sock.async_connect(serverep, boost::bind(&Connection::ConnectCallback, this, _1));
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

void Connection::ConnectCallback(const boost::system::error_code& error)
{
	if (!error)
	{
		DoneConnecting(true, "");
		boost::asio::async_read_until(sock, incomeBuffer, "\n", boost::bind(&Connection::ReceiveCallback, this, _1, _2));
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
		if (error.value() == connection_reset || error.value() == boost::asio::error::eof )
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
		boost::asio::async_read_until(sock, incomeBuffer, "\n", boost::bind(&Connection::ReceiveCallback, this, _1, _2));
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
