#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <string>
#include <random>
#include <netinet/tcp.h>
#include <set>
#include <sys/epoll.h>
#include "test_config.h"
#include <fstream>
#include "getTime.cc"

using std::set;
using std::cout;
using std::endl;
using std::thread;

const int MAXEVENT = 1000;
const int PORT = 7000;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int epfd;
epoll_event evlist[MAXEVENT];


set<int> fds;
int server_sockfd;
bool issending = false;
std::vector<SendConfig> modes;

void init_socket(){
    int flag = 1;
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        printf("socket can not be initialized!\n");
        exit(-1);
    }
    
    setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        close(server_sockfd);
        printf("bind error!\n");
        exit(-1);
    }

    epfd = epoll_create(MAXEVENT);
    epoll_event ev;
    ev.data.fd = server_sockfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,server_sockfd,&ev);

}

int handle_request(std::vector<SendConfig> configs, int mode){
    if(mode < 0 || mode >= configs.size()){
        return -1; // input illegal
    }
    //std::this_thread::sleep_for(std::chrono::seconds(15));

    //std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    SendConfig config = configs[mode];
    int count = 0;

    std::default_random_engine e;
    std::uniform_int_distribution<unsigned> u(config.min_period, config.max_period);
    set<int> tfds = fds;
    while(config.pkt_num -- ){
        if(config.pkt_num % 9 == 0){
            pthread_mutex_lock(&mtx);
            tfds = fds;
            pthread_mutex_unlock(&mtx);
        }
        int gap = u(e); 
        for(int tfd : tfds){
            std::string packet = "PacketNumber:" + std::to_string(count) + " sendTime:" + std::to_string(getMicros()); 
            if(config.big_packet && count % 9 == 0){
                packet.append(3900,'+');
            }
            send(tfd,packet.c_str(),packet.size(),0);
            // scp_send(packet.c_str(),packet.size(),fc);
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
    issending = false;
}

void service_thread(){
    if(listen(server_sockfd,64) < 0){
        cout<<"listen failed"<<endl;
    }
    int recvlen;
    char recvbuffer[4096];
    while(1){
        int evtnum = epoll_wait(epfd,evlist,MAXEVENT,0);
        for(int i = 0; i < evtnum ; i++){
            if(evlist[i].data.fd == server_sockfd){
                int clntfd = accept(server_sockfd,NULL,NULL);
                epoll_event ev;
                ev.data.fd = clntfd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clntfd,&ev);
                pthread_mutex_lock(&mtx);
                fds.insert(clntfd);
                pthread_mutex_unlock(&mtx);
            }else{
                recvlen = recv(evlist[i].data.fd,recvbuffer,4096,0);
                if(recvlen < 10){
                    if(!issending){
                        issending = true;
                        int op = recvbuffer[0]-'0';
                        thread sendingthread(sending_thread,op);
                        sendingthread.detach();
                    } 
                }
            }
        }
    }
}


int main(){

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