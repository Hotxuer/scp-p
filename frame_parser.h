#ifndef FRM_PAR
#define FRM_PAR

#include "header_info.h"
#include "conn_manager.h"

int reply_syn(addr_port src,uint32_t conn_id);


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
// return 8 : heart beat packet
int parse_frame(char* buf, size_t len,uint32_t& conn_id, bool isserver){
    iphead* ip = (iphead*) buf;
    tcphead* tcp = (tcphead*) (buf + sizeof(iphead));
    scphead* scp = (scphead*) (buf + sizeof(iphead) + sizeof(udphead) + sizeof(tcphead));
    addr_port srcaddr = {(in_addr_t)ip->ip_src,tcp->tcp_sport};
    //rmt_ip_port = srcaddr;
    conn_id = scp->connid;
    int scpst;
    if(tcp->tcp_flag == 0x02){ // SYN 报文
        if(! isserver) return -1;
        return reply_syn(srcaddr,conn_id);

    }else if(tcp->tcp_flag == 0x12){ //SYN + ACK , set the local ConnID
        if(isserver) return -1;
        uint32_t localid = ConnidManager::local_conn_id;
        
        if(localid != 0 && localid != conn_id && ConnManager::exist_conn(localid) ){
            ConnManager::del_conn(localid);
        }
        if(localid != conn_id){
            ConnManager::add_conn(conn_id,new FakeConnection(false,srcaddr));
            ConnidManager::local_conn_id = conn_id;
        }
        ConnManager::get_conn(conn_id)->set_conn_id(conn_id);
        ConnManager::get_conn(conn_id)->establish_ok();
        ConnManager::get_conn(conn_id)->update_para(1,1);
        return 3;

    }else if(tcp->tcp_flag == 0x10){ //data 
        if(ConnManager::exist_conn(conn_id)){ // If there is a connection , push it to the connection.
            size_t hdrlen = sizeof(iphead) + sizeof(tcphead);
            scpst = ConnManager::get_conn(conn_id)->on_pkt_recv(buf + hdrlen,len - hdrlen,srcaddr);
            //update tcp-para
            uint16_t needack = htonl((ntohl) (tcp->tcp_seq) + 1); 
            uint16_t needseq = tcp->tcp_ack;
            ConnManager::get_conn(conn_id)->update_para(needseq,needack);
            return 5 + scpst;
        }else{
            // Send a SCP-RESET back. Server do not have the connection.
            return -1;
        }
    }else{
        //其他可能性，暂不考虑
        return 0;
    }
}


int reply_syn(addr_port src,uint32_t conn_id){
    int ret = 1;
    if(ConnManager::exist_addr(src)){
        printf("exist address.\n");
        printf("conn_id : %d.\n",conn_id);
        conn_id = ConnManager::get_connid(src);
        printf("conn_id : %d.\n",conn_id);
    }else if(conn_id == 0 || !ConnManager::exist_conn(conn_id)){ // a new request or the connid not exist
        conn_id = ConnidManager::getConnID();
        ConnManager::add_conn(conn_id,new FakeConnection(true,src));
        ConnManager::get_conn(conn_id)->set_conn_id(conn_id);
        ConnManager::get_conn(conn_id)->establish_ok();
        ConnManager::get_conn(conn_id)->update_para(0,1);
        ConnManager::add_addr(src,conn_id);
        ret = 2;
    }

    unsigned char ackbuf[40];
    headerinfo h = {src.sin,ConnManager::get_local_port(),src.port,0,1,1};
    size_t hdrlen;;
    generate_tcp_packet(ackbuf,hdrlen,h);
    generate_scp_packet(ackbuf+hdrlen,0,0,0,conn_id);

    sockaddr_in rmt_sock_addr;
    
    rmt_sock_addr.sin_family = AF_INET;
    rmt_sock_addr.sin_addr.s_addr = src.sin;
    rmt_sock_addr.sin_port = src.port;

    //printf("port : %d\n",src.port);
    int sz = sendto(ConnManager::local_send_fd,ackbuf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&rmt_sock_addr,sizeof(rmt_sock_addr));
    printf("send sz : %d\n",sz);
    if(sz == -1){
        int err = errno;
        printf("errno %d.\n",err);
    }
    return ret;  
}


#endif

