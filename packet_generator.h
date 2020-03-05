#ifndef PKT_GEN
#define PKT_GEN

#include "header_info.h"

unsigned short cksum(unsigned char* packet, int len){   
    unsigned long sum = 0;
    unsigned short * temp;
    unsigned short answer;
    temp = (unsigned short *)packet;
    unsigned short * endptr = (unsigned short*) (packet+len);
    for( ; temp < endptr; temp += 1)
        sum += *temp;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}


// add tcp & ip header
int generate_tcp_packet(unsigned char* buf, size_t & len,headerinfo info){
    
    //len = sizeof(iphead) + sizeof(tcphead);
    len = sizeof(tcphead);
    //if(info.type != 2) 
        //len -= 4; // no mss option
    //iphead* ip = (iphead*) buf;
    //tcphead* tcp = (tcphead*) (buf+sizeof(iphead));
    tcphead* tcp = (tcphead*)buf;
    memset(buf,0,len);

    //set the ipheader
    /*
    ip->ip_hl = 5;
    ip->ip_version = 4;
    ip->ip_tos = 0;
    ip->ip_len = htons(sizeof(struct iphead) + sizeof(struct tcphead) + info.pktlen);
    ip->ip_id = htons(13542); // random()
    ip->ip_off = htons(0x4000);
    ip->ip_ttl = 64;
    ip->ip_pro = IPPROTO_TCP;
    ip->ip_src = info.src_ip;
    ip->ip_dst = info.dest_ip;
    ip->ip_sum = cksum(buf, 20);  //计算IP首部的校验和，必须在其他字段都赋值后再赋值该字段，赋值前�?

    */

    // set the tcp header
    int my_seq = 0; //TCP序号
    tcp->tcp_sport = info.src_port;
    tcp->tcp_dport = info.dest_port;
    tcp->tcp_seq = htonl(info.seq);
    tcp->tcp_ack = htonl(info.ack);
    if(info.type != 2){
        if(info.type == 0) tcp->tcp_flag = 0x02;  //SYN置位
        else tcp->tcp_flag = 0x12; //SYN和ACK置位
        tcp->tcp_len = 5;  //发送SYN报文段时，设置TCP首部�?4字节(if mss option)
        //tcp->mss_option = 0x0204;
        //tcp->mss = 1460;
    }else{
        tcp->tcp_flag = 0x10;
        tcp->tcp_len = 5;
    }
    tcp->tcp_off = 0;
    tcp->tcp_win = htons(29200);
    tcp->tcp_urp = htons(0);

    /*设置tcp伪首部，用于计算TCP报文段校验和*/
    /*
    struct psdhead psd;
    psd.saddr = info.src_ip; //源IP地址
    psd.daddr = info.dest_ip; //目的IP地址
    psd.mbz = 0;
    psd.ptcl = 6;  
    psd.tcpl = htons(tcp->tcp_len * 4);
    
    unsigned char buffer[100]; //用于存储TCP伪首部和TCP报文，计算校验码
    memcpy(buffer, &psd, sizeof(psd));
    memcpy(buffer+sizeof(psd), tcp, tcp->tcp_len * 4);
    tcp->tcp_sum = cksum(buffer, sizeof(psd) + tcp->tcp_len * 4);
    */
    return 0;
}


// add scp header
int generate_scp_packet(unsigned char* buf,uint8_t type,uint16_t pktnum,uint16_t ack,uint32_t conn_id){
    if(type > 3 || pktnum & 0x8000 || ack & 0x8000) 
        return -1;
    scphead* scp = (scphead *) buf;
    scp -> type = type;
    scp -> pktnum = pktnum;
    scp -> ack = ack;
    scp -> connid = conn_id;
    return 0;
}
#endif
