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
void service_thread(bool isserver){
    int stat;size_t n;
    char recvbuf[4096];
    char sendbuf[256];
    addr_port rmt;
    sockaddr_in rmt_sock_addr;
    int scp_stat;
    headerinfo h;
    while(1){
        n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,NULL,NULL);
        //printf("recv from raw socket, len ,%d\n",n);
        stat = parse_frame(recvbuf + 14,n-14,rmt,isserver);
        switch (stat){
            case 1:
                printf("a request for exist connnection. \n");
            case 2:
                printf("a request from client.\n");
                rmt_sock_addr.sin_addr.s_addr = rmt.sin;
                rmt_sock_addr.sin_port = rmt.port;
        
                h = {ConnManager::get_local_ip(),rmt.sin,ConnManager::get_local_port(),rmt.port,0,1,1,0};
                size_t hdrlen;;
                generate_tcp_packet((unsigned char*)sendbuf,hdrlen,h);
                
                sendto(ConnManager::local_send_fd,sendbuf,hdrlen,0,(struct sockaddr *)&rmt_sock_addr,sizeof(rmt_sock_addr));  
                printf("send back an ack\n");
                break;
            case 3:
                printf("recv a back SYN-ACK from server(not in the server).\n");
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

// filter the packet to port 17001
// tcpdump -dd 'tcp[2:2] == 17001 and tcp[tcpflags] & tcp-rst == 0'
struct sock_filter my_bpf_code[] = {
    { 0x28, 0, 0, 0x0000000c },
    { 0x15, 0, 10, 0x00000800 },
    { 0x30, 0, 0, 0x00000017 },
    { 0x15, 0, 8, 0x00000006 },
    { 0x28, 0, 0, 0x00000014 },
    { 0x45, 6, 0, 0x00001fff },
    { 0xb1, 0, 0, 0x0000000e },
    { 0x48, 0, 0, 0x00000010 },
    { 0x15, 0, 3, 0x00004269 },
    { 0x50, 0, 0, 0x0000001b },
    { 0x45, 1, 0, 0x00000004 },
    { 0x6, 0, 0, 0x00040000 },
    { 0x6, 0, 0, 0x00000000 },
};

size_t send(char* buf,size_t len,FakeConnection* fc){
    //addr_port ta = {rmtaddr,htons(17001)};
    if(!fc) return 0;
    return fc->pkt_send(buf,len);
}


int main(int argc,char** argv){
    int ret = init_rawsocket(my_bpf_code,sizeof(my_bpf_code)/sizeof(struct sock_filter));
    if(ret) printf("init_rawsocket error.");
    //connect(htons(LOCAL_ADDR),htons(REMOTE_ADDR));
    std::thread ser(service_thread,true);
    ser.detach();
    sleep(20);// wait for the connection;
    char str[10] = "hello_scp";
    std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    size_t sendsz;
    for(auto i : v){
        sendsz = send(str,9,i);
        printf("send:--%ld\n",sendsz);
    }
    //size_t sendsz = send(str,9,inet_addr(REMOTE_ADDR));
    
    sleep(10000);
}