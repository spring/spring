#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

#include <iostream>

class RawTextMessage
{
public:
	RawTextMessage(const std::string& _message) : message(_message), pos(0)
	{};
	
	std::string GetWord()
	{
		size_t oldpos = pos;
		pos = message.find_first_of(std::string("\t \n"), oldpos);
		if (pos != std::string::npos)
		{
			return message.substr(oldpos, pos++ - oldpos);
		}
		else if (oldpos != std::string::npos)
		{
			return message.substr(oldpos);
		}
		else
		{
			return "";
		}
	};
	
	int GetInt()
	{
		std::istringstream buf(GetWord());
		int temp;
		buf >> temp;
		return temp;
	};
	
private:
	std::string message;
	size_t pos;
};

class UserCache;

class Connection
{
public:
	Connection();
	~Connection();
	
	void Connect(const std::string& server, int port);
	virtual void DoneConnecting(bool succes, const std::string& err) {};
	virtual void ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode) {};

	void Register(const std::string& name, const std::string& password);
	virtual void RegisterDenied(const std::string& reason) {};
	virtual void RegisterAccept() {};

	void Login(const std::string& name, const std::string& password);
	virtual void Denied(const std::string& reason) {};
	virtual void LoginEnd() {};
	virtual void Aggreement(const std::string text) {};
	void ConfirmAggreement();

	virtual void Motd(const std::string text) {};

	void JoinChannel(const std::string& channame, const std::string& password = "");
	virtual void Joined(const std::string& channame) {};
	virtual void JoinFailed(const std::string& channame, const std::string& reason) {};

	virtual void Disconnected() {};
	virtual void NetworkError(const std::string& msg) {};

	void SendData(const std::string& msg);

	void Poll();
	void Run();

private:
	void DataReceived(const std::string& command, const std::string& msg);

	void ConnectCallback(const boost::system::error_code& error);
	void ReceiveCallback(const boost::system::error_code& error, size_t bytes);
	void SendCallback(const boost::system::error_code& error);
	
	std::string aggreementbuf;
	boost::asio::ip::tcp::socket sock;
	boost::asio::streambuf incomeBuffer;
	UserCache* users;
	boost::posix_time::ptime lastPing;
};

#endif