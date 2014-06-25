OBJ_DIR=./obj
SRC=./src
INC_DIR= \
		 -I./lib/libev-4.15 \
		 -I./lib/json-c \
		 -I./lib/leveldb-1.15.0/include \

LIB_DIR=-L./lib

USE=LIBA
ifeq ($(USE),LIBA)
LIBS=-lpthread -lstdc++ -lm
else
LIBS=-lpthread -lrt -lm
endif

LIBA=-lev -ljson-c -lleveldb -lsnappy
CFLAGS=-Wall -g \
		-DOPEN_PTHREAD \
		-DOPEN_STATIC \
		-DOPEN_COMPRESSION \
		-DOPEN_DEBUG

#CFLAGS=-Wall -O2 \
#		-DOPEN_PTHREAD \
#		-DOPEN_STATIC \
#		-DOPEN_COMPRESSION \

OBJ = $(addprefix $(OBJ_DIR)/, \
      ldb.o \
      main.o \
      read_cfg.o \
      ctl.o \
      utils.o \
      )


help:
	@echo "make libs first!"

libs:
	$(MAKE) -C ./lib clean
	$(MAKE) -C ./lib

all:$(OBJ)
	g++ $(CFLAGS) $^ $(INC_DIR) $(LIB_DIR) $(LIBS) $(LIBA) -o tsdb

$(OBJ_DIR)/%.o:$(SRC)/%.c
	@-if [ ! -d ./obj ];then mkdir obj; fi
	g++ $(CFLAGS) $(INC_DIR) $(LIB_DIR) $(LIBS) -c $< -o $@

run:
	./tsdb &

push:
	git push -u origin master

clean:
	rm -rf tsdb
	rm -rf ./obj/*.o
	@-if [ ! -d ./obj ];then mkdir obj; fi

distclean:
	make clean
	$(MAKE) -C ./lib clean
