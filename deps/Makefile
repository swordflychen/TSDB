LIB_DIR=$(shell pwd)
OBJ = libev.a \
      libjson-c.a \
      libsnappy.a \
      libleveldb.a


all:$(OBJ)
	-@echo "Done LIB!"

libev.a:
	cd ./libev-4.15 && ./configure
	$(MAKE) -C $(LIB_DIR)/libev-4.15
	mv $(LIB_DIR)/libev-4.15/.libs/$@ $(LIB_DIR)

libjson-c.a:
	cd ./json-c && ./autogen.sh && ./configure
	$(MAKE) -C $(LIB_DIR)/json-c
	mv $(LIB_DIR)/json-c/.libs/$@ $(LIB_DIR)

libsnappy.a:
	tar xvf snappy-1.1.1.tar.gz
	cd ./snappy-1.1.1 && ./configure
	$(MAKE) -C $(LIB_DIR)/snappy-1.1.1
	mv $(LIB_DIR)/snappy-1.1.1/.libs/$@ $(LIB_DIR)

libleveldb.a:
	$(MAKE) -C $(LIB_DIR)/leveldb-1.15.0
	mv $(LIB_DIR)/leveldb-1.15.0/$@ $(LIB_DIR)

clean:
	rm -rf $(OBJ)
	$(MAKE) -C $(LIB_DIR)/leveldb-1.15.0 clean
	-if [ -f libev-4.15/Makefile ];then cd ./libev-4.15 && make distclean; else cd ./libev-4.15 && ./configure && make distclean; fi
	-if [ -f json-c/Makefile ];then cd ./json-c && make distclean; fi
	-if [ -d snappy-1.1.1 ];then rm -rf ./snappy-1.1.1; fi

