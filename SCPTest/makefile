all: server client

dependence=../common_lib.h ../header_info.h ../packet_generator.h  ../conn_manager.h ../frame_parser.h ../scp_interface.h

server: $(dependence) SCPTestServer.cc
	g++ -o server $(dependence) SCPTestServer.cc -lpthread

client: $(dependence) SCPTestClient.cc
	g++ -o client $(dependence) SCPTestClient.cc -lpthread

clean:
	rm server client