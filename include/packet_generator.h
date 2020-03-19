#ifndef PKT_GEN
#define PKT_GEN

#include "header_info.h"

int generate_tcp_packet(unsigned char* buf, size_t & len,headerinfo info);
int generate_scp_packet(unsigned char* buf,uint8_t type,uint16_t pktnum,uint16_t ack,uint32_t conn_id);
int generate_udp_packet(unsigned char* buf, uint16_t srcport , uint16_t destport,size_t & len,size_t payload_len);


#endif
