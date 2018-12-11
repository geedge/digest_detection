vpath %.a ../lib
vpath %.h ./inc

PAPP_PATH=/home/mesasoft/dd/

#CFLAGS          =  -g3 -Wall -fPIC -Werror -O
#CFLAGS          =  -g3 -Wall -fPIC -O
INCLUDES        = -I./inc/
INCLUDES        += -I./inc/libevent2/
INCLUDES        += -I./inc/libevent2/event2/
INCLUDES        += -I/usr/include/MESA/
CFLAGS          =  -g3 -Wall -fPIC -D__STDC_FORMAT_MACROS
CFLAGS	       	+= $(INCLUDES)
#LDDICTATOR      = -Wl,-wrap,malloc -Wl,-wrap,calloc -Wl,-wrap,free -Wl,-wrap,realloc
LDDICTATOR      =
CC              = g++
CCC             = g++

LIB		= -L./lib/ 
#LIB		+= -lssl -lcrypto -lxml2 -lcurl -levent -lpthread
LIB		+= -lMESA_handle_logger -lMESA_prof_load -lmaatframe -lrulescan -lMESA_htable -lMESA_field_stat2 -lWiredLB -lpthread
LIB		+= ./lib/libmy_socket.a
LIB		+= ./lib/libtango_cache_client.a ./lib/libcjson.a  ./lib/libcrypto.a  ./lib/libcurl.a  ./lib/libevent.a  ./lib/libhiredis.a  ./lib/libssl.a ./lib/libxml2.a -lz
#LIB		+= ./lib/libdictator_debug.a

LIB_FILE	= $(wildcard ../lib/*.a)
SOURCES		= $(wildcard *.c)
SOURCESCPP  = $(wildcard *.cpp)
OBJECTS 	= $(SOURCES:.c=.o)
OBJECTSCPP  = $(SOURCESCPP:.cpp=.o)
#DEPS		= $(SOURCES:.c=.d)


TARGET		= dd

.PHONY:clean all 

all:$(TARGET)

$(TARGET):$(OBJECTS) $(OBJECTSCPP)  $(LIB_FILE)
	#$(CCC) -shared $(CFLAGS) $(OBJECTS) $(LIB) -o $@
	$(CCC) $(CFLAGS) $(OBJECTS) $(OBJECTSCPP)  $(LIB) $(LDDICTATOR) -o $@
	cp $(TARGET) ../bin/
#	cp $(TARGET) /home/mesasoft/dd/
	
.c.o:
#%.d:%.c
#	$(CCC) $< -MM $(INCLUDES) > $@

.cpp.o:
#%.dpp:%.cpp
	$(CCC) $(CFLAGS) -c $<

-include $(DEPS)

clean :
	rm -f $(OBJECTS) $(DEPS) $(TARGET) 

