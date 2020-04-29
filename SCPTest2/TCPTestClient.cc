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
    // Handle the network-change signal
    // close and delete the old connection
    epoll_ctl(epfd,EPOLL_CTL_DEL,local_sockfd,NULL);
    close(local_sockfd);
    local_sockfd = 0;

    // sleep 3s to reconnect
    std::this_thread::sleep_for(std::chrono::milliseconds(3000)); 

    // get a new fd and reconnect
    local_sockfd = socket(AF_INET,SOCK_STREAM,0);
    
    int flag = 1;
    setsockopt(local_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    connect(local_sockfd, (sockaddr* )&remote_addr , sizeof(remote_addr));

    // add the new clnt-fd to the epoll
    epoll_event ev;
    ev.data.fd = local_sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,local_sockfd,&ev);
}

void rst_thread(pthread_t serviceid){
    // Send a signal to service_thread periodlly , each signal indicate means a network change
    int sigs = 10;
    std::default_random_engine e;
    // send a signal every 10-15s
    std::uniform_int_distribution<unsigned> u(10000, 15000);
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

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);
    remote_addr.sin_port = htons(REMOTE_PORT);

    setsockopt(local_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    if(connect(local_sockfd, (sockaddr* )&remote_addr , sizeof(remote_addr)) != 0){
        cout << "connection failed"<<endl;
    }

    epoll_event ev;
    ev.data.fd = local_sockfd;
    ev.events = EPOLLIN; 
    epoll_ctl(epfd,EPOLL_CTL_ADD,local_sockfd,&ev);

    cout<<"init socket success"<<endl;
}

void service_thread(){
     signal(SIGUSR1,rst_handle);
    // cout<<"in service thread"<<endl;
    int evtnum , recvn;
    logfile.open("testlogs");
    char recvbuffer[4096];
    while(1){
        evtnum = epoll_wait(epfd,evlist,20,-1);
        //cout<<evtnum<<endl;
        for(int i = 0;i < evtnum ; i++){
            int padding = 0;
            uint64_t recvTime = getMicros();
            recvn = recv(evlist[i].data.fd,recvbuffer,4096,0);
            
            if(recvn > 2000){
                padding = 3900;
            }
            //cout<<recvn<<endl;
            if(recvn > 0){
                cout<<"recv:  "<<recvn<<endl;
                std::string data = std::string(recvbuffer,recvn - padding);
                std::string past = data.substr(data.find_last_of(":") + 1);
                uint64_t time = recvTime - std::stoull(past);
                logfile << data << " recvTime:" << recvTime << " timeCost:" << time << endl;
            }else if(recvn == 0){
                // Do some close logic
            }
        }
    }
}

int main(){
    init_socket();
    std::thread ser(service_thread);
    pthread_t serid = ser.native_handle();
    ser.detach();

    // enter the test case num
    int op;
    std::cout << " Enter an interger n to do the test operation or enter -1 to quit."<<std::endl;
    std::cin >> op;

    char msgbuffer[8];
    
    switch(op){
        case -1: 
            close(local_sockfd);
            break;
        default:
            if(op > 7 || op < 0){
                std::cout<<"illegal input."<<std::endl;
            }else{
                std::thread rst(rst_thread,serid);
                rst.detach();
                msgbuffer[0] = '0' + op;
                if(local_sockfd){
                    send(local_sockfd,msgbuffer,2,0);
                    //cout<<errno<<endl;
                }
            }
    }
    pause();
}
