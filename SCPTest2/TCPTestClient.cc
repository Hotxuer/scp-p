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
#include <signal.h>
#include "getTime.cc"

#define REMOTE_IP "202.120.38.131"
#define REMOTE_PORT 7000
using std::cout;
using std::endl;
int epfd , local_sockfd;
epoll_event evlist[20];

sockaddr_in remote_addr;
std::ofstream logfile;

void rst_handle(int sig){
    // close the old connection
    epoll_ctl(epfd,EPOLL_CTL_DEL,local_sockfd,NULL);
    close(local_sockfd);
    local_sockfd = 0;

    std::this_thread::sleep_for(std::chrono::milliseconds(10000)); // 10s to reconnect

    local_sockfd = socket(AF_INET,SOCK_STREAM,0);
    
    int flag = 1;
    setsockopt(local_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    connect(local_sockfd, (sockaddr* )&remote_addr , sizeof(remote_addr));

    epoll_event ev;
    ev.data.fd = local_sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,local_sockfd,&ev);
}

void rst_thread(int serviceid){
    // send a signal to service_thread periodlly 
    int sigs = 10;
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned> u(3000, 6000);
    while(sigs --){
        int gap = u(e); 
        std::this_thread::sleep_for(std::chrono::milliseconds(gap));  
        pthread_kill(serviceid,SIGUSR1);
    }
}

void init_socket(){
    epfd = epoll_create(2);
    local_sockfd = socket(AF_INET,SOCK_STREAM,0);
    
    int flag = 1;
    setsockopt(local_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    if(connect(local_sockfd, (sockaddr* )&remote_addr , sizeof(remote_addr)) != 0){
        cout << "connection failed"<<endl;
    }

    epoll_event ev;
    ev.data.fd = local_sockfd;
    ev.events = EPOLLIN; 
    epoll_ctl(epfd,EPOLL_CTL_ADD,local_sockfd,&ev);
}

void service_thread(){
    signal(SIGUSR1,rst_handle);
    int evtnum , recvn;
    logfile.open("testlogs");
    char recvbuffer[4096];
    while(1){
        evtnum = epoll_wait(epfd,evlist,20,0);
        for(int i = 0;i < evtnum ; i++){
            int padding = 0;
            uint64_t recvTime = getMicros();
            recvn = recv(evlist[i].data.fd,recvbuffer,4096,0);
            if(recvn > 2000){
                padding = 3900;
            }
            std::string data = std::string(recvbuffer,recvn - padding);
            std::string past = data.substr(data.find_last_of(":") + 1);
            uint64_t time = recvTime - std::stoull(past);
            logfile << data << " recvTime:" << recvTime << " timeCost:" << time << '\n';
        }
    }
}

int main(){
    init_socket();
    std::thread ser(service_thread);
    pthread_t serid = ser.native_handle();
    ser.detach();
    
    sleep(2);
    std::thread rst(rst_thread,serid);
    rst.detach();

    int op;
    std::cout << " enter an interger n to do the test operation and enter -1 to quit."<<std::endl;
    char msgbuffer[8];
    while(std::cin >> op){
        bool quit = 0;
        switch(op){
            case -1: 
                //scp_close();
                close(local_sockfd);
                quit = 1;
                break;
            default:
                if(op > 7 || op < 0){
                    std::cout<<"illegal input."<<std::endl;
                }else{
                    //std::string msg = "";
                    //msg += op;
                    msgbuffer[0] = '0' + op;
                    //scp_send(msgbuffer,2,ConnManager::get_conn(ConnidManager::local_conn_id));
                    if(local_sockfd){
                        send(local_sockfd,msgbuffer,2,0);
                    }
                }
        }
        std::cout << " enter an interger n to do the test operation and enter -1 to quit."<<std::endl;
    }
}
