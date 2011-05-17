include ../../Makedefs

LUA_DIR=lua-5.1.4

all: all_liblua

deploy: deploy_liblua

all_liblua:
		make -C $(LUA_DIR) PLAT=posix CC=$(CROSS_COMPILE_ARM)gcc INSTALL_TOP=$(shell pwd) all 
		make -C $(LUA_DIR) PLAT=posix CC=$(CROSS_COMPILE_ARM)gcc INSTALL_TOP=$(shell pwd) install
		
deploy_liblua: all_liblua
		chmod +x lib/liblua.so
		mkdir -p $(WR_INSTALL_ROOT)/lib
		cp lib/liblua.so $(WR_INSTALL_ROOT)/lib
		
clean:
		make -C  $(LUA_DIR) clean