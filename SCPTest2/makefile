all: ./bin/scp_server ./bin/scp_client ./bin/tcp_server ./bin/tcp_client
CC=g++
CFLAGS=-O2

./bin/scp_server: ./obj/scp_server.o ../lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -L /usr/lib/libevent/lib/ -levent -lpthread -lglog -lgflags -lssl -lcrypto
./bin/scp_client: ./obj/scp_client.o ../lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -lpthread -lglog -lgflags -lssl -lcrypto

./bin/tcp_server: ./obj/tcp_server.o
	$(CC) $^ $(CFLAGS) -o $@ -lpthread
./bin/tcp_client: ./obj/tcp_client.o
	$(CC) $^ $(CFLAGS) -o $@ -lpthread

./obj/tcp_server.o: TCPTestServer.cc getTime.cc
	$(CC) $(CFLAGS) -c $< -o $@
./obj/tcp_client.o: TCPTestClient.cc getTime.cc
	$(CC) $(CFLAGS) -c $< -o $@

./obj/scp_server.o: SCPTestServer.cc ../include/scp_interface.h test_config.h
	$(CC) $(CFLAGS) -I /usr/lib/libevent/include -c $< -o $@
./obj/scp_client.o: SCPTestClient.cc ../include/scp_interface.h test_config.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm ./obj/*.o ./bin/*client ./bin/*server