#ifdef _MSC_VER
#include "lib/streflop/streflop_cond.h"
#endif

#include "Connection.h"

#include <bitset>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "md5/md5.h"
#include "md5/base64.h"
#include "UserCache.h"

#if BOOST_VERSION < 103600
using namespace boost::system::posix_error;
#else
using namespace boost::system::errc;
#endif
using namespace boost::asio;
boost::asio::io_service netservice;

std::string RTFToPlain(const std::string& rich)
{
	static const std::string screwTag = "\\pard";
	static const std::string lineend = "\\par";
	std::string out = rich.substr(rich.find_first_of('{')+1, (rich.find_last_of('}') - rich.find_first_of('{')) -2);

	out = out.substr(out.find_last_of('}')+1);
	size_t pos = 0;

	while ((pos = out.find(screwTag)) != std::string::npos)
	{
		out.erase(pos, screwTag.size());
	}
	while ((pos = out.find(lineend)) != std::string::npos)
	{
		out.replace(pos, lineend.size(), "\n");
	}
	while ((pos = out.find('\\')) != std::string::npos)
	{
		size_t pos2 = out.find_first_of("\\ \t\n}", pos+1);
		if (pos2 != std::string::npos)
		{
			out.erase(pos, pos2-pos);
		}
		else
		{
			break;
		}
	}

	return out;
}

Connection::Connection() : sock(netservice), users(new UserCache)
{
}

Connection::~Connection()
{
	delete users;
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
	out << "LOGIN " << name << " " << MD5Base64(password) << " 0 localhost libLobby v0.1\n";
	SendData(out.str());
}

void Connection::ConfirmAggreement()
{
	SendData("CONFIRMAGREEMENT\n");
}

void Connection::JoinChannel(const std::string& channame, const std::string& password)
{
	std::ostringstream out;
	out << "JOIN " << channame;
	if (!password.empty())
		out << " " << password;
	SendData(out.str());
}

void Connection::SendData(const std::string& msg)
{
	if (sock.is_open())
	{
		sock.send(boost::asio::buffer(msg));
	}
	else
	{
		NetworkError("Already disconnected");
	}
}

void Connection::Poll()
{
	const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
	const boost::posix_time::time_duration diff = now - lastPing;
	if (diff.seconds() > 30)
	{
		SendData("PING\n");
		lastPing = now;
	}
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
	else if (command == "LOGININFOEND")
	{
		LoginEnd();
	}
	else if (command == "REGISTRATIONDENIED")
	{
		RegisterDenied(msg);
	}
	else if (command == "REGISTRATIONACCEPTED")
	{
		RegisterAccept();
	}
	else if (command == "ADDUSER")
	{
		if (users)
		{
			RawTextMessage buf(msg);
			std::string name = buf.GetWord();
			UserInfo client = users->Get(name);
			client.country = buf.GetWord();
			client.cpu = buf.GetInt();
			users->AddUser(client);
		}
	}
	else if (command == "REMOVEUSER")
	{
		if (users)
		{
			RawTextMessage buf(msg);
			std::string name = buf.GetWord();
			UserInfo client = users->Get(name);
			users->RemoveUser(client);
		}
	}
	else if (command == "CLIENTSTATUS")
	{
		if (users)
		{
			RawTextMessage buf(msg);
			std::string name = buf.GetWord();
			UserInfo client = users->Get(name);
			std::bitset<8> status(buf.GetInt());
			client.ingame = status[0];
			client.away = status[1];
			client.rank = (status[2]? 1 : 0) + (status[3]? 2 : 0) + (status[4]? 4 : 0);
			client.moderator = status[5];
			client.bot = status[6];
			users->Update(client);
		}
	}
	else if (command == "JOIN")
	{
		if (users)
		{
			RawTextMessage buf(msg);
			std::string channame = buf.GetWord();
			Joined(channame);
		}
	}
	else if (command == "JOINFAILED")
	{
		if (users)
		{
			RawTextMessage buf(msg);
			std::string channame = buf.GetWord();
			std::string reason = buf.GetWord();
			JoinFailed(channame, reason);
		}
	}
	else if (command == "AGREEMENT")
	{
		aggreementbuf += msg + "\n";
	}
	else if (command == "AGREEMENTEND")
	{
		
		Aggreement(RTFToPlain(aggreementbuf));
	}
	else if (command == "MOTD")
	{
		Motd(msg);
	}
	else
	{
		// std::cout << "Unhandled command: " << command << " " << msg << std::endl;
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
