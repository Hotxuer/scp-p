#include <thread>
#include "../include/scp_interface.h"
#include "test_config.h"
#include <fstream>
#include <event2/event.h>
#include <event2/event_struct.h>

#include <stdlib.h>

#define LOCAL_PORT_USED 17001
//#define REMOTE_PORT_USED 17000

#define LOCAL_ADDR "202.120.38.131"
//#define REMOTE_ADDR "202.120.38.100"

bool issending = 0;
std::vector<SendConfig> modes;

int handle_request(std::vector<SendConfig> configs, int mode){
    if(mode < 0 || mode >= configs.size()){
        return -1; // input illegal
    }
    //std::this_thread::sleep_for(std::chrono::seconds(15));

    std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    SendConfig config = configs[mode];
    int count = 0;

    std::default_random_engine e;
    std::uniform_int_distribution<unsigned> u(config.min_period, config.max_period);
    while(config.pkt_num -- ){
        int gap = u(e); 
        for(FakeConnection* fc : v){
            std::string packet = "PacketNumber:" + std::to_string(count) + " sendTime:" + std::to_string(getMicros()); 
            if(config.big_packet && count % 9 == 0){
                packet.append(3900,'+');
            }
            scp_send(packet.c_str(),packet.size(),fc);
        }
        count++;
        //int prd = get_random_sleeptime(config.min_period,config.max_period);
        std::this_thread::sleep_for(std::chrono::milliseconds(gap));  
    }
}

void sending_thread(int mode){
    if(handle_request(modes, mode) < 0){
       // exit(mode);
       std::cout<<"illegal request"<<std::endl;
    }  
    issending = 0;
}

struct event_args{
    char recvbuf[4096];
    char sendbuf[4096];
    //addr_port rmt;
    bool isserver;
};

void handle_event(evutil_socket_t listener, short event, void *arg){
    event_args* ev_args = (event_args*) arg;
    size_t n;

    addr_port src;
    bool tcpenable = ConnManager::tcp_enable; 

    struct sockaddr_in fromAddr;
    socklen_t fromAddrLen = sizeof(fromAddr);
    uint32_t this_conn_id;
    int stat;

    if(ConnManager::tcp_enable){
        n = recvfrom(listener,ev_args->recvbuf,4096,0,NULL,NULL);
        stat = parse_frame(ev_args->recvbuf + 14,n-14,this_conn_id,src);
    }else{
        n = recvfrom(listener,ev_args->recvbuf,4096,0,(struct sockaddr*)&fromAddr,&fromAddrLen);
        src.sin = fromAddr.sin_addr.s_addr;
        src.port = fromAddr.sin_port;
        stat = parse_frame(ev_args->recvbuf ,n,this_conn_id,src);        
    }

    switch (stat){
        case 1:
            LOG(INFO) << "a request for exist connnection.";
                //printf("a request for exist connnection. \n");
            break;
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
    if(stat == 6){
        // record the ack-message
    }
    if(stat == 7){
        int md = ev_args->recvbuf[8] - '0';
        if(!issending){
            issending = 1;
            std::cout<<"start sending mode : "<<md<<std::endl;
            std::thread sendthread(sending_thread,md);
            sendthread.detach();
        }
    }  
}

void service_thread(){
    evutil_socket_t listen_raw_socket = (evutil_socket_t) ConnManager::local_recv_fd;
    struct event_base* base;
    struct event *listener_event;

    event_args evarg;
    evarg.isserver = 1;
    base = event_base_new();
    listener_event = event_new(base,listen_raw_socket,EV_READ|EV_PERSIST,handle_event,&evarg);

    event_add(listener_event,NULL);
    event_base_dispatch(base);
}


int main()
{
    int ret = init_rawsocket(false, true);
    if(ret) printf("init_rawsocket error.");
    
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    printf("bind ok!\n");


    std::ifstream conf("config.conf");
    std::string title;

    getline(conf,title);
    int pktnum , prdmin , prdmax , isbig;
    
    while(conf >> pktnum){
        conf >> prdmin >> prdmax >> isbig;
        SendConfig conf = { pktnum , prdmin , prdmax , isbig};
        modes.push_back(conf);
    }
    std::thread ser(service_thread);
    ser.detach();

    int mode;
    std::cout<<"---------------------------------"<<std::endl;
    std::cout<<" -1 : exit \n -2 : query config mode \n other : send packet with mode n \n"<<std::endl;
    std::cout<<"---------------------------------"<<std::endl;
    bool cont = 1;
    while(std::cin >> mode){
        switch(mode){
            case -1 : 
                cont = 0;
                break;
            case -2 : 
                query_config(modes);
                break;
            default:
                issending = 1;
                if(handle_request(modes, mode) < 0){
                    exit(mode);
                }
                issending = 0;
        }
        if(!cont) break;
        std::cout<<"---------------------------------"<<std::endl;
        std::cout<<" -1 : exit \n 0 : query config mode \n other : send packet with mode n \n"<<std::endl;
        std::cout<<"---------------------------------"<<std::endl;
    }

    std::cout<<"server will stop in 10s"<<std::endl;
    sleep(10);
    //close server 
    return 0;
}
