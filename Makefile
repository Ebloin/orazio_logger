PREFIX=orazio_lib
CC=gcc
INCLUDE_DIRS=-I$(PREFIX)/src/common -I$(PREFIX)/src/orazio_host/ -I$(PREFIX)/src/host_test/
CC_OPTS=-Wall -Ofast --std=gnu99 $(INCLUDE_DIRS)

LIBS=-lpthread -lreadline -lwebsockets -ljpeg

LOBJS = packet_handler.o\
	deferred_packet_handler.o\
	orazio_client.o\
	orazio_print_packet.o\
	orazio_client_test_getkey.o\
	serial_linux.o


HEADERS = packet_header.h\
	packet_operations.h\
	packet_handler.h\
	deferred_packet_handler.h\
	orazio_packets.h\
	orazio_print_packet.h\
	capture.h\
	queue.h\
	webcam_manager.h

BINS=orazio_logger


.phony: clean all

all:    $(BINS)

#common objects
%.o:    $(PREFIX)/src/common/%.c
		$(CC) $(CC_OPTS) -c  $<

%.o:    $(PREFIX)/src/orazio_host/%.c
		$(CC) $(CC_OPTS) -c  $<

%.o:    $(PREFIX)/src/host_test/%.c
		$(CC) $(CC_OPTS) -c  $<

%.o:    $(PREFIX)/src/orazio_/%.c
		$(CC) $(CC_OPTS) -c  $<

#logger
orazio_logger:  orazio_logger.c $(LOBJS)
		$(CC) $(CC_OPTS) -o $@ $^ $(LIBS)

clean:
		rm -rf $(OBJS) $(BINS) *~ *.d *.o
