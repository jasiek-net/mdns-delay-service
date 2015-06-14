CC	= cc
CFLAGS	= -Wall -O2
LFLAGS	= -Wall

TARGET = main

DEPT = udp_server.o udp_client.o tcp_client.o icmp.o dropnobody.o in_cksum.o telnet.o mdns.o mdns_extra.o err.o linkedlist.o 

all: $(TARGET)

mdns: mdns.o mdns_extra.o
	$(CC) $(LFLAGS) $^ -o $@ -lpthread

main: main.o $(DEPT)
	$(CC) $(LFLAGS) $^ -o $@ -lpthread
	rm -f *.o

udp: udp_server_test.o err.o err.h
	$(CC) $(LFLAGS) $^ -o $@

tcp: tcp_server_test.o err.o err.h
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean all
clean:
	rm -f $(TARGET) *.o *~ *.bak
