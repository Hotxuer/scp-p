#include "packet_generator.h"
#include "frame_parser.h"

int scp_bind(in_addr_t localip , uint16_t port);
int init_rawsocket(bool tcpenable, bool isserver);
int scp_connect(in_addr_t remote_ip,uint16_t remote_port);
size_t scp_send(const char* buf,size_t len,FakeConnection* fc);
int scp_close();