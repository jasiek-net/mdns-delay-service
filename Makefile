CC	= cc
CFLAGS	= -Wall -O2
LFLAGS	= -Wall

TARGET = main

DEPT = err.o linkedlist.o mdns.o udp_client.o udp_server.o tcp_client.o

all: $(TARGET)

mdns: mdns.o
	$(CC) $(LFLAGS) $^ -o $@ -lpthread

main: main.o $(DEPT)
	$(CC) $(LFLAGS) $^ -o $@ -lpthread
	rm -f *.o

udp: udp_server_test.o err.o err.h
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean all
clean:
	rm -f $(TARGET) *.o *~ *.bak
