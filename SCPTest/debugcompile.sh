#!/bin/bash
<<<<<<< HEAD
dependence='../common_lib.h ../header_info.h ../packet_generator.h  ../conn_manager.h ../frame_parser.h ../scp_interface.h'

g++ -g -o ./Debug/server $dependence SCPTestServer.cc -lpthread

g++ -g -o ./Debug/client $dependence SCPTestClient.cc -lpthread
=======
dependence='common_lib.h header_info.h packet_generator.h  conn_manager.h frame_parser.h scp_interface.h'

g++ -g -o ./DebugTest/server $dependence server.cc -lpthread

g++ -g -o ./DebugTest/client $dependence client.cc -lpthread

g++ -g -o ./DebugTest/server_libevent $dependence server_libevent.cc -I /usr/lib/libevent/include -L /usr/lib/libevent/lib/ -levent -lpthread 
>>>>>>> 0ebfc228aade9d4e301d628c8915b8d0d27a47de
