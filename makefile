all: server client server_libevent

dependence=common_lib.h header_info.h packet_generator.h  conn_manager.h frame_parser.h

server: $(dependence) server.cc
	g++ -o server $(dependence) server.cc -lpthread

client: $(dependence) client.cc
	g++ -o client $(dependence) client.cc -lpthread

server_libevent: $(dependence) server_libevent.cc
	g++ -o server_libevent $(dependence) server_libevent.cc -I /usr/lib/libevent/include -L /usr/lib/libevent/lib/ -levent -lpthread 

clean:
	rm server client server_libevent
 