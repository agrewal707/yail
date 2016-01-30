#ifndef YAIL_LOG_H
#define YAIL_LOG_H

#include <iostream>

#if !defined(YAIL_LOGGER)
#define YAIL_LOGGER std::clog
#endif

#define DO_YAIL_LOG(logger, msg)  				\
	do {                                    \
		logger << __FUNCTION__ << "(): "      \
		       << msg << std::endl;    \
	} while (false);

#define DO_YAIL_LOG_FUNCTION(logger, msg)  \
	do {                                     \
		std::stringstream tmp;                 \
		yail::parameter_logger p(&tmp);        \
		p << msg;                              \
		logger << __FUNCTION__ << "(): "       \
		       << tmp.str () << std::endl;     \
	} while (false)

#define YAIL_LOG_WARNING(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)
#define YAIL_LOG_ERROR(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)

#if defined(YAIL_DEBUG)
#define YAIL_LOG_DEBUG(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)
#else // YAIL_DEBUG
#define YAIL_LOG_DEBUG(msg)
#endif

#if defined(YAIL_TRACE)
#include <sstream>
#define YAIL_LOG_TRACE(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)
#define YAIL_LOG_FUNCTION(msg) DO_YAIL_LOG_FUNCTION(YAIL_LOGGER, msg)
#else // YAIL_TRACE
#define YAIL_LOG_TRACE(msg)
#define YAIL_LOG_FUNCTION(msg)
#endif

#if defined(YAIL_TRACE)
namespace yail {

class parameter_logger: public std::ostream
{
public:
	explicit parameter_logger (std::ostream* os) : 
		std::basic_ostream<char>(),
		m_os (os),
		m_item_number (0)
	{}

	template<typename T>
	parameter_logger& operator<< (T param) 
	{
		switch (m_item_number)
		{
		case 0: // first parameter
			(*m_os) << param;
			break;
		default: // parameter following a previous parameter
			(*m_os) << ", " << param;
			break;
		}
		m_item_number++;
		return *this;
	}

private:
	std::ostream* m_os;
	int32_t m_item_number;
};

} // namespace yail
#endif // YAIL_TRACE


#endif // YAIL_LOG_H
