#ifndef YAIL_LOG_H
#define YAIL_LOG_H

#include <iostream>

#if !defined(YAIL_LOGGER)
#define YAIL_LOGGER std::clog
#endif

#define DO_YAIL_LOG(logger, msg)  	\
	do {                                    \
		logger << __FUNCTION__ << "(): "      \
		       << msg << std::endl;           \
	} while (false);

#define YAIL_LOG_WARNING(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)
#define YAIL_LOG_ERROR(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)

#if defined(YAIL_DEBUG)
#define YAIL_LOG_DEBUG(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)
#else // YAIL_DEBUG
#define YAIL_LOG_DEBUG(msg)
#endif

#if defined(YAIL_TRACE)
#define YAIL_LOG_TRACE(msg) DO_YAIL_LOG(YAIL_LOGGER, msg)
#else // YAIL_TRACE
#define YAIL_LOG_TRACE(msg)
#endif

#endif // YAIL_LOG_H
