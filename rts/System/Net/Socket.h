#ifndef SOCKET_H
#define SOCKET_H

#include <boost/asio/io_service.hpp>


namespace netcode
{

extern boost::asio::io_service netservice;
bool CheckErrorCode(boost::system::error_code&);

} // namespace netcode

#endif
