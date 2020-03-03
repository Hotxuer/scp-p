#define CLNT_MODE
#include "packet_generator.h"
#include "frame_parser.h"
#include <cstring>

#define LOCAL_PORT_USED 17000
#define REMOTE_PORT_USED 17001

#define LOCAL_ADDR "202.120.38.100"
#define REMOTE_ADDR "202.120.38.131"

//int recv_rawsockfd , send_rawsockfd;
//sockaddr_in server_addr,clnt_addr;

//char sendbuf[2000];
//char recvbuf[2000];


// init a raw socket

int init_rawsocket(struct sock_filter* bpf_code, size_t code_items ){
    if(ConnManager::local_send_fd != 0 || ConnManager::local_send_fd != 0){
        // This method should only be active once.
        return -1;
    }

    struct sock_fprog filter;

    //filter.len = sizeof(bpf_code)/sizeof(struct sock_filter); 
    filter.len = code_items;
    filter.filter = bpf_code;

    // initial recv_fd,收到的为以太网帧，帧头长度为18字节
    int recv_rawsockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (recv_rawsockfd < 0 ) {
        //perror("socket fail\n");
        printf("recv socket initial failed\n");
        return -1;
    }else{
        ConnManager::local_recv_fd = recv_rawsockfd;
    }
    //设置 sk_filter
    if (setsockopt(recv_rawsockfd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) < 0) {
        //perror("setsockopt fail\n"); 
        printf("setsockopt fail failed\n");
        return -1;  
    }

    //initial send_fd
    int send_rawsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(send_rawsockfd < 0){
        printf("send socket initial failed\n");
        return -1;
    }else{
        ConnManager::local_send_fd = send_rawsockfd;
    }
    int one = 1;
    if(setsockopt(send_rawsockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0){   //定义套接字不添加IP首部，代码中手工添加
        printf("setsockopt failed!\n");
        return -1;
    }

    return 0;
}

// connect , send TCP-SYN ,wait for TCP-SYN back
int connect(in_addr_t local_ip,in_addr_t remote_ip){

    sockaddr_in local_addr, server_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(17000);
    local_addr.sin_addr.s_addr = local_ip;
    ConnManager::set_local_addr(local_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(17001);
    server_addr.sin_addr.s_addr = remote_ip;
    
    addr_port remote_ad_pt = {remote_ip , htons(17001)};
    // add to the local_conn_manager.
    ConnManager::add_conn(remote_ad_pt,new FakeConnection(false,remote_ad_pt));

    // send syn
    headerinfo h= {ConnManager::get_local_ip(),remote_ip,ConnManager::get_local_port(),remote_ad_pt.port,0,0,0,0};
    size_t hdrlen;
    unsigned char tmp_send_buf[45];
    generate_tcp_packet(tmp_send_buf,hdrlen,h);
    sendto(ConnManager::local_send_fd,tmp_send_buf,hdrlen,0,(struct sockaddr *)&server_addr,sizeof(server_addr));

    printf("send syn ok\n"); 
    // recv syn-ack
    char recvbuf[128];
    int n = recvfrom(ConnManager::local_recv_fd, recvbuf, 1024, 0, NULL, NULL); 
    
    addr_port rmt;
    if(parse_frame(recvbuf+14,n-14,rmt,false) != 3){// not include ethheader.
        printf("illegal frame in\n");
        return -1;
    } else{
        printf("recv server ack ,len : %d\n",n);
    }
    // send empty message with ack
    /*
    h.type = 2; h.seq = 1; h.ack = 1;
    generate_tcp_packet((unsigned char*)tmp_send_buf,hdrlen,h);
    sendto(ConnManager::local_send_fd,tmp_send_buf,hdrlen,0,(struct sockaddr *)&server_addr,sizeof(server_addr)); 
    printf("send ack ok\n"); 
    */ 
    return 0;
}


void service_thread(bool isserver){
    int stat;size_t n;
    char recvbuf[4096];
    addr_port rmt;
    int scp_stat;
    while(1){
        n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,NULL,NULL);
        stat = parse_frame(recvbuf + 14,n-14,rmt,isserver);
        switch (stat){
            case 1:
            case 2:
                printf("This should not be a client packet.\n");
                break;
            case 3:
                printf("recv a back SYN-ACK from server.\n");
                break;
            case 4:
                printf("recv a reset request.\n");
                break;
            case 5:
                printf("recv a scp redundent ack.\n");
                break;
            case 6:
                printf("recv a scp packet ack.\n");
                break;
            case 7:
                printf("recv a scp data packet.\n");
                break;
            case -1:
                printf("recv a illegal frame.\n");
                break;
            default:
                break;
        }
        // n = recvfrom(recv_rawsockfd, recvbuf, 1024, 0, NULL, NULL); 
        // stat = parse_frame(recvbuf + 14,n-14);
        
    }
}

// filter the packet to port 17000
// tcpdump -dd 'tcp[2:2] == 17000 and tcp[tcpflags] & tcp-rst == 0'
struct sock_filter my_bpf_code[] = {
    { 0x28, 0, 0, 0x0000000c },
    { 0x15, 0, 10, 0x00000800 },
    { 0x30, 0, 0, 0x00000017 },
    { 0x15, 0, 8, 0x00000006 },
    { 0x28, 0, 0, 0x00000014 },
    { 0x45, 6, 0, 0x00001fff },
    { 0xb1, 0, 0, 0x0000000e },
    { 0x48, 0, 0, 0x00000010 },
    { 0x15, 0, 3, 0x00004268 },
    { 0x50, 0, 0, 0x0000001b },
    { 0x45, 1, 0, 0x00000004 },
    { 0x6, 0, 0, 0x00040000 },
    { 0x6, 0, 0, 0x00000000 },
};

size_t send(char* buf,size_t len,in_addr_t rmtaddr){
    addr_port ta = {rmtaddr,htons(17001)};
    return ConnManager::get_conn(ta)->pkt_send(buf,len);
}

int reset_ip(in_addr_t now_addr, in_addr_t rmtaddr){
    // send a scp resend packet to the server if now_addr not equal to the previous one
}

int close(in_addr_t rmtaddr){

}

int main(int argc,char** argv){
    int ret = init_rawsocket(my_bpf_code,sizeof(my_bpf_code)/sizeof(struct sock_filter));
    if(ret) printf("init_rawsocket error.");
    connect(inet_addr(LOCAL_ADDR),inet_addr(REMOTE_ADDR));
    std::thread ser(service_thread,false);
    ser.detach();
    //char* str = "hello_scp";
    //size_t sendsz = send(str,strlen(str),inet_addr(REMOTE_ADDR));
    //printf("send:--%d",sendsz);
    sleep(10000);
}