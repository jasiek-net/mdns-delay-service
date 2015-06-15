CC	= cc
CFLAGS	= -Wall -O2
LFLAGS	= -Wall

TARGET = opoznienia

DEPT = udp_server.o udp_client.o tcp_client.o icmp.o dropnobody.o in_cksum.o telnet.o mdns.o mdns_extra.o err.o linkedlist.o 

all: $(TARGET)

opoznienia: opoznienia.o $(DEPT)
	$(CC) $(LFLAGS) $^ -o $@ -lpthread
	rm -f *.o

.PHONY: clean all
clean:
	rm -f $(TARGET) *.o *~ *.bak
