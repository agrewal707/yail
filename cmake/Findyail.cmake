##############################################################################
# Looks for the yail library installation and defines:
#
#  yail_FOUND - system has yail.
#  yail_INCLUDE_DIRS - the yail include directory.
#  yail_LIBRARIES - Link these to use yail.
#
# You need to define the YAIL_INSTALL_DIR variable or have the
# environment variable $YAIL_INSTALL_DIR to be set to your yail
# installation directory.
##############################################################################

# If YAIL_INSTALL_DIR was defined in the environment, use it.
MESSAGE(STATUS "V: ${YAIL_INSTALL_DIR}")

if (NOT YAIL_INSTALL_DIR AND NOT $ENV{YAIL_INSTALL_DIR} STREQUAL "")
  set(YAIL_INSTALL_DIR $ENV{YAIL_INSTALL_DIR})
endif(NOT YAIL_INSTALL_DIR AND NOT $ENV{YAIL_INSTALL_DIR} STREQUAL "")

FIND_PATH(yail_INCLUDE_DIR
	NAMES
		yail/io_service.h
	PATHS
		${YAIL_INSTALL_DIR}/include
)

IF (yail_INCLUDE_DIR)
	MESSAGE(STATUS "Found yail include dir: ${yail_INCLUDE_DIR}")
ELSE (yail_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "Could not find yail include dir")
ENDIF (yail_INCLUDE_DIR)

SET(yail_INCLUDE_DIRS 
	${yail_INCLUDE_DIR} 
)

# Find libraries
FIND_LIBRARY(yail_LIBRARY
	NAMES
		yail
	PATHS
		${YAIL_INSTALL_DIR}/lib
)

SET(yail_LIBRARIES
		${yail_LIBRARY}
)


IF (yail_INCLUDE_DIRS AND yail_LIBRARIES)
	SET(yail_FOUND TRUE)
ENDIF (yail_INCLUDE_DIRS AND yail_LIBRARIES)

IF (yail_FOUND)
	MESSAGE(STATUS "Found yail C++ libraries: ${yail_LIBRARIES}")
ELSE (yail_FOUND)
	IF (yail_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find yail")
	ENDIF (yail_FIND_REQUIRED)
ENDIF (yail_FOUND)

MARK_AS_ADVANCED(yail_INCLUDE_DIRS yail_LIBRARIES)


