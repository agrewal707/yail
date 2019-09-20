##############################################################################
# Looks for the boost_asio library installation and defines:
#
#  boost_asio_FOUND - system has boost_asio.
#  boost_asio_INCLUDE_DIRS - the boost_asio include directory.
#  boost_asio_LIBRARIES - Link these to use boost_asio.
#
# You need to define the BOOST_ASIO_INSTALL_DIR variable or have the
# environment variable $BOOST_ASIO_INSTALL_DIR to be set to your boost_asio
# installation directory.
##############################################################################

# If BOOST_ASIO_INSTALL_DIR was defined in the environment, use it.
MESSAGE(STATUS "INSTALL_DIR: ${BOOST_ASIO_INSTALL_DIR}")

if (NOT BOOST_ASIO_INSTALL_DIR AND NOT $ENV{BOOST_ASIO_INSTALL_DIR} STREQUAL "")
  set(BOOST_ASIO_INSTALL_DIR $ENV{BOOST_ASIO_INSTALL_DIR})
endif(NOT BOOST_ASIO_INSTALL_DIR AND NOT $ENV{BOOST_ASIO_INSTALL_DIR} STREQUAL "")

# Find libraries
FIND_LIBRARY(boost_asio_LIBRARY
	NAMES
		boost_asio
	PATHS
		${BOOST_ASIO_INSTALL_DIR}/lib
	NO_CMAKE_FIND_ROOT_PATH
)

SET(boost_asio_LIBRARIES
		${boost_asio_LIBRARY}
)

IF (boost_asio_LIBRARIES)
	SET(boost_asio_FOUND TRUE)
ENDIF (boost_asio_LIBRARIES)

IF (boost_asio_FOUND)
	MESSAGE(STATUS "Found boost_asio C++ libraries: ${boost_asio_LIBRARIES}")
ELSE (boost_asio_FOUND)
	IF (boost_asio_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find boost_asio")
	ENDIF (boost_asio_FIND_REQUIRED)
ENDIF (boost_asio_FOUND)

MARK_AS_ADVANCED(boost_asio_LIBRARIES)
