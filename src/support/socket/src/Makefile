LIB_PATH=./
SRC=./
OBJ=./
INCLUDE=-I ./inc
INCLUDE+=-I /usr/include/MESA/

LIB=-L./lib  -lsitdc++ -lpthread 
CC=g++ -g

G_FLAG_SO = -Wall -fPIC $(INCLUDE)
G_FLAG = -Wall $(INCLUDE)

SRCFILE= $(wildcard  $(SRC)*.c)
DETACHDIR= $(notdir $(SRCFILE))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(DETACHDIR)) 

TARGETLIB = $(LIB_PATH)libmy_socket.a
TARGETSO  = $(LIB_PATH)libmy_socket.so


$(OBJ)%.o:$(SRC)%.c
	$(CC) -c -o $@ $(G_FLAG_SO)  $<

all:$(TARGETLIB) 
#all:$(TARGETSO) 

$(TARGETLIB):$(OBJECTS)
	ar rs $@  $^
	cp $@ ../bin/
	cp $@ ../../../lib/
	cp my_socket.h  ../../../inc/
$(TARGETSO):$(OBJECTS)
	$(CC) -o $@ $(G_FLAG) -shared $^
	cp $@ ../bin/
	cp $@ ../../../lib/
	cp my_socket.h	../../../inc/
clean:
	-rm -rf $(OBJECTS) $(TARGETSO) $(TARGETLIB) *.o 
install:
	cp *.a ../../../lib/

.PHONY:all clean
