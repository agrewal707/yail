#
# YAIL Makefile
#

src = $(CURDIR)
obj = $(subst $(srctree),$(objtree),$(src))

# default confd path if not configured
include $(srctree)/$(BSCORE)/rules/config.mk
# quiet output
include $(srctree)/$(BSCORE)/rules/quiet.mk
include $(srctree)/$(BSCORE)/rules/funcs.mk
# bscore build system
include $(srctree)/$(BSCORE)/rules/toolchain.mk
#include $(srctree)/$(BSCORE)/rules/buildflags.mk

LIBNAME = yail
TARGETS = $(addprefix $(obj)/,yail/$(SHAREDLIB))

# library install location(s)
INSTALL_PREFIX = $(installroot)/usr
INSTALL_DIRS = $(INSTALL_PREFIX)/lib

# library install targets
INSTALL_TARGETS = $(INSTALL_DIRS)/lib$(LIBNAME).so

# shared library
SHAREDLIB = lib$(LIBNAME).so

VERBOSE=0

CMAKE_FLAGS=\
	-DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
	-DCMAKE_SYSTEM_NAME=Linux \
	-DCMAKE_SYSTEM_VERSION=1 \
	-DCMAKE_C_COMPILER=$(TOOLCHAIN_TARGET_PATH)/$(CC) \
	-DCMAKE_CXX_COMPILER=$(TOOLCHAIN_TARGET_PATH)/$(CXX) \
	-DCMAKE_FIND_ROOT_PATH=$(TOOLCHAIN_SYSROOT_DIR) \
	-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
	-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
	-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
	-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
	-DPROTOBUF_PROTOC_EXECUTABLE=$(TOOLCHAIN_TARGET_PATH)/$(PROTOC) \
	-DProtobuf_PROTOC_EXECUTABLE=$(TOOLCHAIN_TARGET_PATH)/$(PROTOC) \
	-DYAIL_BUILD_BOOST_ASIO_LIBRARY=no \
	-DBOOST_ASIO_INSTALL_DIR=$(installroot)/usr \
	-DCMAKE_BUILD_TYPE=MinSizeRel

# uncomment when yail logger has been enhanced to use cafe logger
#ifeq (y,$(${KCONFIG_ITEM_PREFIX}BUILD_DEBUG))
#	CMAKE_FLAGS += -DCMAKE_BUILD_TYPE=Debug
#else
#	CMAKE_FLAGS += -DCMAKE_BUILD_TYPE=MinSizeRel
#endif

all: | buildpaths installpaths
	$(Q)cd $(obj); cmake $(CMAKE_FLAGS) $(src) $(REDIRECT_OUTPUT)
	$(Q)$(MAKE) -s all_install

all_install:
	$(Q)$(MAKE) -s -C $(obj) install/strip $(REDIRECT_OUTPUT)

install: | installpaths
	$(quiet_msg_install)
	$(Q)$(MAKE) -s all_install

uninstall:
	$(quiet_msg_uninstall)
	$(Q)rm -rf $(installroot)/usr/lib/lib$(LIBNAME).so*

clean:
	$(quiet_msg_clean)
	$(Q)rm -rf $(obj)/*

buildpaths :
	$(Q)mkdir -p $(obj)

installpaths:
	$(Q)mkdir -p $(INSTALL_DIRS)

testoutput:

.PHONY: clean buildpaths installpaths

# build targets

# install targets

$(INSTALL_TARGETS) : | buildpaths installpaths

# catchall
%::
	@:
