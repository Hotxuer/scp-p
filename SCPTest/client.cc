#define CLNT_MODE
#include "scp_interface.h"
#include <cstring>

#define LOCAL_PORT_USED 17000
#define REMOTE_PORT_USED 17001

#define LOCAL_ADDR "202.120.38.100"
#define REMOTE_ADDR "202.120.38.131"

// init a raw socket
void service_thread(bool isserver){
    int stat;size_t n;
    char recvbuf[4096];
    //addr_port rmt;
    uint32_t this_conn_id;

    //int scp_stat;
    addr_port src;
    bool tcpenable = ConnManager::tcp_enable;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrLen = sizeof(fromAddr);

    while(1){
        if(tcpenable){
            n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,NULL,NULL);
            stat = parse_frame(recvbuf + 14,n-14,this_conn_id,src);
        }else{
            n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,(struct sockaddr*)&fromAddr,&fromAddrLen);
            src.sin = fromAddr.sin_addr.s_addr;
            src.port = fromAddr.sin_port;
            stat = parse_frame(recvbuf ,n,this_conn_id,src);
        }
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

int main(int argc,char** argv){
    int ret = init_rawsocket(false, false);
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    std::thread ser(service_thread,false);
    ser.detach();
    sleep(2);
    ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
    if(ret == 0){
        printf("connect to server failed.\n");
    }
    
    sleep(10000);
}