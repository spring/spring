#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
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
	
private:
	std::string message;
	size_t pos;
};

class Connection
{
public:
	Connection();
	~Connection();
	
	void Connect(const std::string& server, int port);
	virtual void DoneConnecting(bool succes, const std::string& err) {};
	virtual void Disconnected() {};

	void SendData(const std::string& msg);
	virtual void DataReceived(const std::string& command, const std::string& msg) {};

	virtual void NetworkError(const std::string& msg) {};

	void Poll();
	void Run();

private:
	void ConnectCallback(const boost::system::error_code& error);
	void ReceiveCallback(const boost::system::error_code& error, size_t bytes);
	void SendCallback(const boost::system::error_code& error);
	
	boost::asio::ip::tcp::socket sock;
	boost::asio::streambuf incomeBuffer;
};

#endif