#include "include/frame_parser.h"


//void wait_reply_syn_ack(addr_port src, uint32_t conn_id);

//int reply_close(addr_port src, uint32_t conn_id);

int parse_tcp_frame(char* buf, size_t len,uint32_t& conn_id,addr_port& srcaddr){
    iphead* ip = (iphead*) buf;
    tcphead* tcp = (tcphead*) (buf + sizeof(iphead));
    scphead* scp = (scphead*) (buf + sizeof(iphead) + sizeof(udphead) + sizeof(tcphead));
    srcaddr = {(in_addr_t)ip->ip_src,tcp->tcp_sport};
    int total_head = sizeof(iphead) + sizeof(udphead) + sizeof(tcphead);
    return parse_scp_frame(buf+total_head, len-total_head, conn_id, srcaddr);
    //rmt_ip_port = srcaddr;
    // conn_id = scp->connid;
    // int scpst;
    // //server端收到syn报文，回复syn+ack报文
    // if(tcp->tcp_flag == 0x02){ // SYN 报文
    //     if(! ConnManager::isserver) return -1;
    //     size_t hdrlen = sizeof(iphead) + sizeof(tcphead) + sizeof(udphead);
    //     ConnManager::get_conn(conn_id)->on_pkt_recv(buf + hdrlen,len - hdrlen,srcaddr);
    //     return reply_syn(srcaddr,conn_id);
    // }//client端收到syn+ack报文，回复三次握手报文，并且设置established = 1
    // else if(tcp->tcp_flag == 0x12){ //SYN + ACK , set the local ConnID
    //     if (ConnManager::isserver)
    //         return -1;
    //     if (!ConnManager::exist_conn(conn_id) || !ConnManager::get_conn(conn_id)->is_established()) {
    //     {   //第一次收到syn+ack
    //         if(ConnManager::isserver) return -1;
    //         uint32_t localid = ConnidManager::local_conn_id;
        
    //         if(localid != 0 && localid != conn_id && ConnManager::exist_conn(localid) ){
    //             ConnManager::del_conn(localid);
    //         }
    //         if(localid != conn_id){
    //         ConnManager::add_conn(conn_id,new FakeConnection(srcaddr));
    //         ConnidManager::local_conn_id = conn_id;
    //         }
    //         ConnManager::get_conn(conn_id)->set_conn_id(conn_id);
    //         ConnManager::get_conn(conn_id)->establish_ok();
    //         ConnManager::get_conn(conn_id)->update_para(1,1);
    //     }   
    //     return reply_syn_ack(srcaddr, conn_id);

    // }else if(tcp->tcp_flag == 0x10){ //data 
    //     if(ConnManager::exist_conn(conn_id)){ // If there is a connection , push it to the connection.
    //         size_t hdrlen = sizeof(iphead) + sizeof(tcphead) + sizeof(udphead);
    //         scpst = ConnManager::get_conn(conn_id)->on_pkt_recv(buf + hdrlen,len - hdrlen,srcaddr);
    //         //update tcp-para
    //         uint16_t needack = htonl((ntohl) (tcp->tcp_seq) + 1); 
    //         uint16_t needseq = tcp->tcp_ack;
    //         ConnManager::get_conn(conn_id)->update_para(needseq,needack);
    //         return 5 + scpst;
    //     }else{
    //         // Send a SCP-RESET back. Server do not have the connection.
    //         return -1;
    //     }
    // }else{
    //     //其他可能性，暂不考虑
    //     return 0;
    // }
}

int parse_scp_frame(char* buf, size_t len,uint32_t& conn_id, addr_port& srcaddr){
    scphead* scp = (scphead*) buf;
    conn_id = scp->connid;
    uint16_t scp_pkt_num,scp_ack_num,scp_type;
    scp_type = scp->type;
    scp_pkt_num = scp->pktnum;
    scp_ack_num = scp->ack;

    int scpst;

    if(scp_type != 1){ // not syn or fin
        if (!ConnManager::get_conn(conn_id)) {
            reply_close(srcaddr, conn_id);
            return -1;
        }
        if (ConnManager::get_conn(conn_id)->is_established()) {
            scpst = ConnManager::get_conn(conn_id)->on_pkt_recv(buf,len,srcaddr);
        } else {
            //recv packet when not established!
            //reply_syn(srcaddr, scp->connid);
            scpst = -5;
        }
        return 5 + scpst;
    }

    if(scp_pkt_num == 0x7fff && scp_ack_num == 0){ // syn
        if(! ConnManager::isserver) return -1;
        scpst = reply_syn(srcaddr,conn_id);
        ConnManager::get_conn(conn_id)->on_pkt_recv(buf,len,srcaddr);
        return scpst;        
    }else if(scp_pkt_num == 0 && scp_ack_num == 0x7fff){ //syn-ack
        if(ConnManager::isserver) return -1;
        //client端收到二次握手报�?
        uint32_t localid = ConnidManager::local_conn_id;
        
        if(localid != 0 && localid != conn_id && ConnManager::exist_conn(localid) ){
            ConnManager::del_conn(localid);
        }
        if(localid != conn_id){
            ConnManager::add_conn(conn_id,new FakeConnection(srcaddr));
            ConnidManager::local_conn_id = conn_id;
        }
        ConnManager::get_conn(conn_id)->set_conn_id(conn_id);
        ConnManager::get_conn(conn_id)->establish_ok();
        return reply_syn_ack(srcaddr, conn_id);
        //ConnManager::get_conn(conn_id)->update_para(1,1);        
    }else if(scp_pkt_num == 0x7fff && scp_ack_num == 0x7fff){ // 3-shakehand from cli to ser
        //server端收到第三次握手报文
        ConnManager::get_conn(scp->connid)->establish_ok();
        return 4;
    }else if(scp_pkt_num == 0 && scp_ack_num == 0){ // close packet
        if (ConnManager::isserver && ConnManager::exist_conn(conn_id)) {
            //server recv close packet
            FakeConnection *conn = ConnManager::get_conn(scp->connid);
            ConnManager::del_addr(conn->get_addr());
            ConnManager::del_conn(conn->get_conn_id());
            LOG(INFO) << "server close a client connect, conn_id: " << conn_id;
        } else if (!ConnManager::isserver) {
            //client recv close packet
            FakeConnection *conn = ConnManager::get_conn(scp->connid);
            conn->establish_rst();
            close(ConnManager::local_send_fd);
            if (ConnManager::tcp_enable)
                close(ConnManager::local_recv_fd);
            ConnManager::del_addr(conn->get_addr());
            ConnManager::del_conn(conn->get_conn_id());
            ConnManager::local_recv_fd = ConnManager::local_send_fd = 0;
            ConnManager::min_rtt = 0;
            LOG(INFO) << "client close finish!";
        }
        // } else {
        //     //对于client端收到应答，关闭一切东�?
        //     //若close报文丢弃，则让清理dead_conn自动清理即可
        //     close(ConnManager::local_send_fd);
        //     if (ConnManager::tcp_enable)
        //         close(ConnManager::local_recv_fd);
        //     del_conn(conn_id);
        //     ConnManager::conn.clear();
        //     ConnManager::addr_pool.clear();
        //     ConnManager::local_recv_fd = ConnManager::local_send_fd = 0;
        // }
        return 9;
    }else{ // error
        return -1;
    }

}

// parse an ip packet
// return -1 : error
// return 0 : recv a data pkt when not established
// return 1 : a request for connect with exist connection
// return 2 : a new connection established on server
// return 3 : client recv a tcp pkt with SYN-ACK
// return 4 : server recv a pkt with reply-syn-ack
// return 5 : data--scp redundent ack
// return 6 : data--legal ack
// return 7 : data--data packet
// return 8 : heart beat packet
// return 9 : echo heart beat packet
// return 10: close packet
// return 11: close echo packet 
int parse_frame(char* buf, size_t len,uint32_t& conn_id,addr_port& srcaddr){
    if(ConnManager::tcp_enable){
        return parse_tcp_frame(buf,len,conn_id,srcaddr);
    }else{
        return parse_scp_frame(buf,len,conn_id,srcaddr);
    }
}


// int reply_close(addr_port src,uint32_t conn_id) {
//     unsigned char ackbuf[40];

//     headerinfo h = {src.sin,ConnManager::get_local_port(),src.port,0,0,2};
//     size_t hdrlen = 0;

//     if(ConnManager::tcp_enable){
//         generate_tcp_packet(ackbuf,hdrlen,h);
//         generate_udp_packet(ackbuf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
//         //myseq += sizeof(scphead) + sizeof(udphead);
//     }
//     generate_scp_packet(ackbuf + hdrlen,1,0,0,conn_id);

//     sockaddr_in rmt_sock_addr;
    
//     rmt_sock_addr.sin_family = AF_INET;
//     rmt_sock_addr.sin_addr.s_addr = src.sin;
//     rmt_sock_addr.sin_port = src.port;

//     //printf("port : %d\n",src.port);
//     int sz = sendto(ConnManager::local_send_fd,ackbuf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&rmt_sock_addr,sizeof(rmt_sock_addr));
//     printf("send sz : %d\n",sz);
//     if(sz == -1){
//         int err = errno;
//         printf("errno %d.\n",err);
//     }
//     return 0;  
// }



// void wait_reply_syn_ack(addr_port src,uint32_t conn_id) {
//     uint32_t sleep_time = 10000, max_wait = 5;
//     std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
//     if (!ConnManager::exist_conn(conn_id) || !ConnManager::exist_addr(src) 
//         || ConnManager::get_conn(conn_id)->is_established() || !(max_wait--))
//         return;
//     reply_syn(src, conn_id);
//     sleep_time <<= 1;
// }
