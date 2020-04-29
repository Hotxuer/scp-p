// #include "../TCPTest/getTime.cc"
#include <thread>
#include <unordered_map>
#include <fstream>
#include "../include/scp_interface.h"
#include <signal.h>
#include <random>

#define LOCAL_PORT_USED 17000
#define REMOTE_PORT_USED 17001

#define LOCAL_ADDR "202.120.38.100"
#define REMOTE_ADDR "202.120.38.131"

std::unordered_map<std::string, uint64_t> testData;

int pktNum1, pktNum2, pktNum3;

std::ofstream logfile;

void rst_handle(int sig){ 
    // handle the signal
    int ret;
    ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
    if(ret == 0){
        printf("connect to server failed.\n");
    }
}


void rst_thread(int serviceid){
    // send a signal to service_thread periodlly 
    int sigs = 10;
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned> u(3000, 6000);
    while(sigs --){
        int gap = u(e); 
        std::this_thread::sleep_for(std::chrono::milliseconds(gap));  
        //pthread_kill(serviceid,SIGUSR1);
    }
}

void service_thread(){
    signal(SIGUSR1,rst_handle);
    int stat;size_t n;
    char recvbuf[4096];
    uint32_t this_conn_id;
    int recv_packets = 0;
    testData.clear();
    int headlen = ConnManager::tcp_enable ? 70 : 8; 

    addr_port src;
    bool tcpenable = ConnManager::tcp_enable;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrLen = sizeof(fromAddr);

    logfile.open("testlogs");

    while(ConnManager::local_recv_fd){
        uint64_t recvTime = getMicros();
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
            case 0:
                printf("recv a data pkt when not established\n");
            case 1:
                printf("a request for exist connnection. \n");
            case 2:
                printf("a request from client.\n");
                break;
            case 3:
                printf("recv a back SYN-ACK from server(not in the server).\n");
                break;
            case 4:
                printf("server recv a pkt with reply-syn-ack.\n");
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
            case 8:
                printf("recv heart beat packet.\n");
            case 9:
                printf("recv close packet.\n");
            case -1:
                printf("recv a illegal frame.\n");
                break;
            default:
                break;
        }
        if(stat == 7){
            int padding = 0;
            if(n > 2000) padding = 3900;
            std::string data = std::string(recvbuf + headlen,n - headlen - padding);
            std::string past = data.substr(data.find_last_of(":") + 1);
            //std::cout << i->first << "   " << past << std::endl;
            uint64_t time = recvTime - std::stoull(past);
            logfile << data << " recvTime:" << recvTime << " timeCost:" << time << '\n';
        }  
    }
}


int main(int argc, char const *argv[])
{
    // pktNum1 = pktNum2 = pktNum3 = 1000;
    // if (argc >= 2)
    //     pktNum1 = atoi(argv[1]);
    // if (argc >= 3)
    //     pktNum2 = atoi(argv[2]);
    // if (argc >= 4)
    //     pktNum3 = atoi(argv[3]);

    int ret = init_rawsocket(false, false);
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    std::thread ser(service_thread);
    pthread_t serid = ser.native_handle();
    ser.detach();


    sleep(2);
    ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
    if(ret == 0){
        printf("connect to server failed.\n");
    }
    
    std::thread rst(rst_thread,serid);
    rst.detach();
    int op;
    std::cout << " enter an interger n to do the test operation and enter -1 to quit."<<std::endl;
    char msgbuffer[8];
    while(std::cin >> op){
        bool quit = 0;
        switch(op){
            case -1: 
                scp_close();
                quit = 1;
                break;
            default:
                if(op > 7 || op < 0){
                    std::cout<<"illegal input."<<std::endl;
                }else{
                    //std::string msg = "";
                    //msg += op;
                    msgbuffer[0] = '0' + op;
                    msgbuffer[1] = '\0';
                    std::cout<<"op -- "<<msgbuffer[0]<<std::endl;
                    scp_send(msgbuffer,2,ConnManager::get_conn(ConnidManager::local_conn_id));
                }
        }
        std::cout << " enter an interger n to do the test operation and enter -1 to quit."<<std::endl;
    }

    sleep(300);
    scp_close();
    sleep(10);
    return 0;
}
