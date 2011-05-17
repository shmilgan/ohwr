
LUA_DIR=lua-5.1.4

all:
	$(MAKE) -C $(LUA_DIR) PLAT=posix \
	   CC=$(CROSS_COMPILE_ARM)gcc INSTALL_TOP=$(shell pwd) all 

install: all
	$(MAKE) -C $(LUA_DIR) PLAT=posix \
		CC=$(CROSS_COMPILE_ARM)gcc INSTALL_TOP=$(shell pwd) install
	chmod +x lib/liblua.so
	mkdir -p $(WR_INSTALL_ROOT)/lib
	cp lib/liblua.so $(WR_INSTALL_ROOT)/lib

# on clean, also remove the installation into the local directory
clean:
	$(MAKE) -C  $(LUA_DIR) clean
	rm -rf bin include lib man
