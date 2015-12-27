#ifndef YAIL_IO_SERVICE_H
#define YAIL_IO_SERVICE_H

#include <yail/config.h>

#if defined (YAIL_USES_BOOST_ASIO)
#include <boost/asio.hpp>
#endif

namespace yail {

#if defined (YAIL_USES_BOOST_ASIO)
using io_service = boost::asio::io_service;
#endif

} // namespace yail

#endif // YAIL_IO_SERVICE_H
