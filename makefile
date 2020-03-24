#all: obj/scp_interface.o obj/conn_manager.o obj/frame_parser.o obj/packet_generator.o obj/conn_id_manager.o obj/common_lib.o obj/server.o obj/client.o obj/server_libevent.o lib/scplib.a bin/server bin/client bin/server_libevent
all: bin/server_libevent bin/server bin/client lib/scplib.a 
CC=g++
CFLAGS=-O2

bin/server_libevent: obj/server_libevent.o lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -L /usr/lib/libevent/lib/ -levent -lpthread -lglog -lgflags
bin/server: obj/server.o lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -lpthread -lglog -lgflags
bin/client: obj/client.o lib/scplib.a
	$(CC) $^ $(CFLAGS) -o $@ -lpthread -lglog -lgflags

lib/scplib.a: obj/scp_interface.o obj/conn_manager.o obj/frame_parser.o obj/packet_generator.o obj/conn_id_manager.o obj/common_lib.o
	ar -cr $@ $^
obj/client.o: client.cc include/scp_interface.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/server.o: server.cc include/scp_interface.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/server_libevent.o: server_libevent.cc include/scp_interface.h
	$(CC) $(CFLAGS) -I /usr/lib/libevent/include -c $< -o $@


obj/scp_interface.o: scp_interface.cc include/scp_interface.h include/frame_parser.h include/packet_generator.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/conn_manager.o: conn_manager.cc include/packet_generator.h include/conn_id_manager.h include/conn_manager.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/frame_parser.o: frame_parser.cc include/header_info.h include/conn_manager.h include/conn_manager.h include/frame_parser.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/packet_generator.o: packet_generator.cc include/header_info.h include/packet_generator.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/conn_id_manager.o: conn_id_manager.cc include/conn_id_manager.h
	$(CC) $(CFLAGS) -c $< -o $@
obj/common_lib.o: common_lib.cc include/common_lib.h
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm bin/* lib/* obj/*
 
