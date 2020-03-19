all: server client
CC=g++
CFLAGS=-O2

server: server.o ../lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -lpthread
client: client.o ../lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -lpthread
server.o: SCPTestServer.cc ../include/scp_interface.h
	$(CC) $(CFLAGS) -c $< -o $@
client.o: SCPTestServer.cc ../include/scp_interface.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o client server

