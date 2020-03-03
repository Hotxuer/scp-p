#ifndef FRM_PAR
#define FRM_PAR

#include "header_info.h"
#include "conn_manager.h"

// parse an ip packet
// return -1 : error
// return 0 : this function did nothing
// return 1 : a request for connect with exist connection
// return 2 : a new connection established on server
// return 3 : client recv a tcp pkt with SYN-ACK
// return 4 : data--scp reset request
// return 5 : data--scp redundent ack
// return 6 : data--legal ack
// return 7 : data--data packet


int parse_frame(char* buf, size_t len,addr_port& rmt_ip_port , bool isserver){
    iphead* ip = (iphead*) buf;
    tcphead* tcp = (tcphead*) (buf + sizeof(iphead));
    addr_port srcaddr = {(in_addr_t)ip->ip_src,tcp->tcp_sport};
    rmt_ip_port = srcaddr;
    int scpst;
    if(tcp->tcp_flag == 0x02){ // SYN 报文
        if(! isserver) return -1;
        if( ConnManager::exist_conn(srcaddr)){
            return 1;
        }else{
            ConnManager::add_conn(srcaddr,new FakeConnection(true,srcaddr));
            ConnManager::get_conn(srcaddr)->establish_ok();
            ConnManager::get_conn(srcaddr)->update_para(0,1);
            return 2;
        }
    }else if(tcp->tcp_flag == 0x12){ //SYN + ACK
        if(isserver) return -1;
        if(!ConnManager::exist_conn(srcaddr)){
            return -1;
        }else{
            ConnManager::get_conn(srcaddr)->establish_ok();
            ConnManager::get_conn(srcaddr)->update_para(1,1);
            return 3;
        }
    }else if(tcp->tcp_flag == 0x10){ //data 报文,带ack
        if(ConnManager::exist_conn(srcaddr)){
            size_t hdrlen = sizeof(iphead) + sizeof(tcphead) - 4;
            scpst = ConnManager::get_conn(srcaddr)->on_pkt_recv(buf + hdrlen,len - hdrlen);
            uint16_t needack = htonl((ntohl) (tcp->tcp_seq) + 1); 
            uint16_t needseq = tcp->tcp_ack;
            ConnManager::get_conn(srcaddr)->update_para(needseq,needack);
            return 5 + scpst;
        }else{
            return -1;
        }
    }else{
        //其他可能性，暂不考虑
        return 0;
    }

}
#endif