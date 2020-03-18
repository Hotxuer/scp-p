#!/bin/bash
dependence='../common_lib.h ../header_info.h ../packet_generator.h  ../conn_manager.h ../frame_parser.h ../scp_interface.h'

g++ -g -o ./Debug/server $dependence SCPTestServer.cc -lpthread

g++ -g -o ./Debug/client $dependence SCPTestClient.cc -lpthread
