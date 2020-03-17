#!/bin/bash
dependence='common_lib.h header_info.h packet_generator.h  conn_manager.h frame_parser.h scp_interface.h'

g++ -g -o ./DebugTest/server $dependence server.cc -lpthread

g++ -g -o ./DebugTest/client $dependence client.cc -lpthread

g++ -g -o ./DebugTest/server_libevent $dependence server_libevent.cc -I /usr/lib/libevent/include -L /usr/lib/libevent/lib/ -levent -lpthread 