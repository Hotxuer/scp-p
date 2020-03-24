#include "include/scp_interface.h"

#include <cstring>

#define LOCAL_PORT_USED 17001
//#define REMOTE_PORT_USED 17000

#define LOCAL_ADDR "202.120.38.131"
#define REMOTE_ADDR "202.120.38.100"


int pktNum1, pktNum2, pktNum3;
//int recv_rawsockfd , send_rawsockfd;
//sockaddr_in server_addr,clnt_addr;


void service_thread(bool isserver){
    LOG(INFO) << "start service thread";
    int stat;size_t n;
    char recvbuf[4096];
    uint32_t this_conn_id;
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
        LOG_IF(ERROR, n <= 0) << "recv error, errno :" << errno;
        //LOG_IF(INFO, n > 0) << "recv from socket, len " << n;
        //printf("recv from raw socket, len ,%d\n",n);
        switch (stat){
            case 1:
                LOG(INFO) << "a request for exist connnection.";
                break;
                //printf("a request for exist connnection. \n");
            case 2:
                LOG(INFO) << "a request for client.";
                //printf("a request from client.\n");
                break;
            case 3:
                LOG(INFO) << "recv a back SYN-ACK from server(not in the server).";
                // printf("recv a back SYN-ACK from server(not in the server).\n");
                break;
            case 4:
                LOG(INFO) << "server recv a pkt with reply-syn-ack.";
                // printf("recv a reset request.\n");
                break;
            case 5:
                LOG(INFO) << "recv a scp redundent ack.";
                // printf("recv a scp redundent ack.\n");
                break;
            case 6:
                LOG(INFO) << "recv a scp packet ack.";
                // printf("recv a scp packet ack.\n");
                break;
            case 7:
                LOG(INFO) << "recv a scp data packet.";
                // printf("recv a scp data packet.\n");
                break;
            case 8:
                LOG(INFO) << "recv heart beat packet.";
                // printf("recv heart beat packet.\n");
                break;
            case 9:
                LOG(INFO) << "recv close packet.";
                // printf("recv close packet.\n");
                break;
            case -1:
                LOG(WARNING) << "recv a illegal frame.";
                // printf("recv a illegal frame.\n");
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


int main(int argc,char** argv){

    init_glog(argv[0], "../logs/");
    int ret = init_rawsocket(false, true);
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    //connect(htons(LOCAL_ADDR),htons(REMOTE_ADDR));
    std::thread ser(service_thread,true);
    ser.detach();
    
    sleep(20);// wait for the connection;


    char str[10] = "hello_scp";
//    printf("here.\n");
    LOG(INFO) << "here";
    std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    size_t sendsz;
//    printf("now_link %ld\n",v.size());
    LOG(INFO) << "now_link" << v.size();
    for(auto i : v){
        sendsz = scp_send(str,9,i);
        printf("send:--%ld\n",sendsz);
    }
    
    //size_t sendsz = send(str,9,inet_addr(REMOTE_ADDR));


    sleep(10000);
}