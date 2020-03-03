#ifndef HEADER_INFO
#define HEADER_INFO

#include "common_lib.h"

struct iphead{       //IP首部
    unsigned char ip_hl:4, ip_version:4;
    unsigned char ip_tos;
    uint16_t ip_len;
    uint16_t ip_id;
    uint16_t ip_off;
    uint8_t ip_ttl;
    uint8_t ip_pro;
    uint16_t ip_sum;
    uint32_t ip_src;
    uint32_t ip_dst;
};

struct tcphead{      //TCP首部
    uint16_t tcp_sport;
    uint16_t tcp_dport;
    uint32_t tcp_seq;
    uint32_t tcp_ack;
    unsigned char tcp_off:4, tcp_len:4;
    uint8_t tcp_flag;
    uint16_t tcp_win;
    uint16_t tcp_sum;
    uint16_t tcp_urp;
    // add mss option when shake hand , deperacate
    uint16_t mss_option;
    uint16_t mss;
};


struct psdhead{ //TCP伪首�?
    unsigned int saddr; //源地址
    unsigned int daddr; //目的地址
    unsigned char mbz;//置空
    unsigned char ptcl; //协议类型
    unsigned short tcpl; //TCP长度
};


struct TcpHeaderInfo {
    in_addr_t src_ip;
    in_addr_t dest_ip;
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    int type; //type 0: shake_hand , type 1: shake_hand_ack , type 2: data.
    uint16_t pktlen;
};

typedef struct iphead iphead;
typedef struct tcphead tcphead;
typedef struct TcpHeaderInfo headerinfo;


struct scphead{
    // type 0: ack, type 1: reset, type 2: data
    uint32_t type:2,pktnum:15,ack:15;
};
#endif