OBJ_DIR=./obj
LIB_DIR=./deps
SRC=./src
TARGET=tsdb
PREFIX=/usr/local/tsdb

# CFLAGS=-Wall -O2 -DOPEN_PTHREAD -DOPEN_STATIC -DOPEN_COMPRESSION
CFLAGS=-Wall -g -DOPEN_PTHREAD -DOPEN_STATIC -DOPEN_COMPRESSION -DOPEN_DEBUG

INC_DIR= \
		 -I$(LIB_DIR)/libev-4.15 \
		 -I$(LIB_DIR)/json-c \
		 -I$(LIB_DIR)/leveldb-1.15.0/include \

LNK_DIR=-L$(LIB_DIR)

USE=LIBA
ifeq ($(USE),LIBA)
LIBS=-lpthread -lstdc++ -lm
else
LIBS=-lpthread -lrt -lm
endif

LIBA=-lev -ljson-c -lleveldb -lsnappy

OBJ = $(addprefix $(OBJ_DIR)/, \
      ldb.o \
      main.o \
      read_cfg.o \
      ctl.o \
      utils.o \
	  logger.o \
      )

help:
	@echo "make libs first!"

libs:
	$(MAKE) -C $(LIB_DIR) clean
	$(MAKE) -C $(LIB_DIR)

all:$(OBJ)
	g++ $(CFLAGS) $^ $(INC_DIR) $(LNK_DIR) $(LIBS) $(LIBA) -o $(TARGET)

$(OBJ_DIR)/%.o:$(SRC)/%.c
	@-if [ ! -d $(OBJ_DIR) ];then mkdir $(OBJ_DIR); fi
	g++ $(CFLAGS) $(INC_DIR) $(LNK_DIR) $(LIBS) -c $< -o $@

install:
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/var/logs
	cp config.json tsdb tsdb_start.sh tsdb_stop.sh $(PREFIX)

push:
	git push -u origin master

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJ_DIR)/*.o
	@-if [ ! -d $(OBJ_DIR) ];then mkdir $(OBJ_DIR); fi

distclean:
	make clean
	$(MAKE) -C $(LIB_DIR) clean

.PHONY: help libs all install push clean distclean
