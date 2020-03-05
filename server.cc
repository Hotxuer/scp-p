#include "packet_generator.h"
#include "frame_parser.h"
#include <cstring>

#define LOCAL_PORT_USED 17001
//#define REMOTE_PORT_USED 17000

#define LOCAL_ADDR "202.120.38.131"
#define REMOTE_ADDR "202.120.38.100"

//int recv_rawsockfd , send_rawsockfd;
//sockaddr_in server_addr,clnt_addr;

//char sendbuf[2000];
//char recvbuf[2000];
int scp_bind(in_addr_t localip , uint16_t port){
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = localip;
    local_addr.sin_port = htons(port);

    ConnManager::set_local_addr(local_addr);

    int sendfd = ConnManager::local_send_fd;
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

    my_bpf_code[8] = { 0x15, 0, 3, port & 0xffffffff};

    struct sock_fprog filter;
    filter.filter = my_bpf_code;
    filter.len = sizeof(my_bpf_code)/sizeof(struct sock_filter);

    if (setsockopt(ConnManager::local_recv_fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) < 0) {
        //perror("setsockopt fail\n"); 
        printf("setsockopt fail failed\n");
        return -1;  
    }

    if(sendfd != 0){
        return bind(sendfd,(sockaddr*) &local_addr,sizeof(local_addr));
    }
}


// init a raw socket

int init_rawsocket(){
    if(ConnManager::local_send_fd != 0 || ConnManager::local_send_fd != 0){
        // This method should only be active once.
        return -1;
    }
    // initial recv_fd,收到的为以太网帧，帧头长度为18字节
    int recv_rawsockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (recv_rawsockfd < 0 ) {
        //perror("socket fail\n");
        printf("recv socket initial failed\n");
        return -1;
    }else{
        ConnManager::local_recv_fd = recv_rawsockfd;
    }

    //initial send_fd
    int send_rawsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(send_rawsockfd < 0){
        printf("send socket initial failed\n");
        return -1;
    }else{
        ConnManager::local_send_fd = send_rawsockfd;
    }
    /*
    int one = 1;
    
    if(setsockopt(send_rawsockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0){   //定义套接字不添加IP首部，代码中手工添加
        printf("setsockopt failed!\n");
        return -1;
    }
    */

    return 0;
}

void service_thread(bool isserver){
    int stat;size_t n;
    char recvbuf[4096];
    uint32_t this_conn_id;
    while(1){
        n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,NULL,NULL);
        //printf("recv from raw socket, len ,%d\n",n);
        stat = parse_frame(recvbuf + 14,n-14,this_conn_id,isserver);
        switch (stat){
            case 1:
                printf("a request for exist connnection. \n");
            case 2:
                printf("a request from client.\n");
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


size_t send(char* buf,size_t len,FakeConnection* fc){
    //addr_port ta = {rmtaddr,htons(17001)};
    if(!fc) return 0;
    return fc->pkt_send(buf,len);
}


int main(int argc,char** argv){
    int ret = init_rawsocket();
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    //connect(htons(LOCAL_ADDR),htons(REMOTE_ADDR));
    std::thread ser(service_thread,true);
    ser.detach();
    
    sleep(20);// wait for the connection;
    char str[10] = "hello_scp";
    printf("here.\n");
    std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    size_t sendsz;
    printf("now_link %ld\n",v.size());
    for(auto i : v){
        sendsz = send(str,9,i);
        printf("send:--%ld\n",sendsz);
    }
    
    //size_t sendsz = send(str,9,inet_addr(REMOTE_ADDR));
    
    sleep(10000);
}