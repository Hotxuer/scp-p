#include "../TCPTest/getTime.cc"
#include <thread>
#include "../scp_interface.h"
#define LOCAL_PORT_USED 17001
//#define REMOTE_PORT_USED 17000

#define LOCAL_ADDR "202.120.38.131"
#define REMOTE_ADDR "202.120.38.100"

int pktNum1, pktNum2, pktNum3;

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

void send_thread(){
    int count = 0;
    const int prd = 100000;
    std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    FakeConnection* fc = v[0];
    for(int i = 0;i < pktNum1 ; i++){
        std::string packet = "PacketNumber:" + std::to_string(count++) + " sendTime:" + std::to_string(getMicros()); 
        scp_send(packet.c_str(),packet.size(),v[0]);
        std::this_thread::sleep_for(std::chrono::microseconds(prd));
    }
    printf("finish test1,test2 will start in 5 seconds.\n");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    count = 0;
    for(int i = 0;i < pktNum2; i++){
        std::string packet = "PacketNumber:" + std::to_string(count++) + " sendTime:" + std::to_string(getMicros()); 
        scp_send(packet.c_str(),packet.size(),v[0]);
        std::this_thread::sleep_for(std::chrono::microseconds(prd));
        //usleep(prd);        
    }
    printf("finish test2,test3 will start in 5 seconds.\n");
    sleep(5);
    count = 0;
    for(int i = 0;i < pktNum3; i++){
        std::string packet = "PacketNumber:" + std::to_string(count++) + " sendTime:" + std::to_string(getMicros()); 
        scp_send(packet.c_str(),packet.size(),v[0]);
        std::this_thread::sleep_for(std::chrono::microseconds(prd));
       // usleep(prd);          
    }
}


int main(int argc, char const *argv[])
{
    pktNum1 = pktNum2 = pktNum3 = 1000;
    if (argc >= 2)
        pktNum1 = atoi(argv[1]);
    if (argc >= 3)
        pktNum2 = atoi(argv[2]);
    if (argc >= 4)
        pktNum3 = atoi(argv[3]);

    int ret = init_rawsocket();
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    //connect(htons(LOCAL_ADDR),htons(REMOTE_ADDR));
    std::thread ser(service_thread,true);
    ser.detach();
    std::this_thread::sleep_for(std::chrono::seconds(20));
   // sleep(20);// wait for the connection;

    std::thread sendthread(send_thread);
    sendthread.join();

    //close server 
    return 0;
}
