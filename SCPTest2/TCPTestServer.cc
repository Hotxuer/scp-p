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
#include <signal.h>
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

// send() to a disconnected fd may recv a SIGPIPE from kernel, ignore it.
void ingnore_sigpipe(int sig){
    //cout<<"may send to a disconnected socket"<<endl;
}

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

// Choose a sending mode from configs
int handle_request(std::vector<SendConfig>& configs, int mode){

    if(mode < 0 || mode >= configs.size()){
        return -1; // input illegal
    }
    //std::this_thread::sleep_for(std::chrono::seconds(15));
    SendConfig config = configs[mode];
    int count = 0;

    std::default_random_engine e;
    std::uniform_int_distribution<unsigned> u(config.min_period, config.max_period);
    pthread_mutex_lock(&mtx);
    set<int> tfds = fds;
    pthread_mutex_unlock(&mtx);
    //cout<<"fds :"<<tfds.size()<<endl;
    while(config.pkt_num -- ){
        // update the tfds each 5 packet 
        if(config.pkt_num % 5 == 0){
            pthread_mutex_lock(&mtx);
            //cout<<"fds:"<<fds.size()<<endl;
            tfds = fds;
            pthread_mutex_unlock(&mtx);
        }
        int gap = u(e); 
        // cout<<"tfds : "<<tfds.size()<<endl;
        for(int tfd : tfds){
            std::string packet = "PacketNumber:" + std::to_string(count) + " sendTime:" + std::to_string(getMicros()); 
            if(config.big_packet && (count % 9 == 0)){
                packet.append(3900,'+');
            }
            cout<<"before send"<<endl;
            int sz = send(tfd,packet.c_str(),packet.size(),0);
            cout<<"after send sended  "<<sz<<endl;
        }
        
        cout<<"cnt: "<<count<<endl;
        count++;
        //int prd = get_random_sleeptime(config.min_period,config.max_period);
        std::this_thread::sleep_for(std::chrono::milliseconds(gap));  
    }
}

// A sending thread created by client test request
void sending_thread(int mode){
    signal(SIGPIPE,ingnore_sigpipe);
    if(handle_request(modes, mode) < 0){
       // exit(mode);
       std::cout<<"illegal request"<<std::endl;
    }  
    issending = false;
    cout<<"sending thread finish"<<endl;
}

void service_thread(){
    if(listen(server_sockfd,64) < 0){
        cout<<"listen failed"<<endl;
        //cout<<errno<<endl;
    }
    int recvlen;
    char recvbuffer[4096];
    int clntfd;
    while(1){
        //cout<<"before epoll wait"<<endl;
        int evtnum = epoll_wait(epfd,evlist,MAXEVENT,-1);
        for(int i = 0; i < evtnum ; i++){
            if(evlist[i].data.fd == server_sockfd){
                cout<<"recv a connection request"<<endl;
                clntfd = accept(server_sockfd,NULL,NULL);
                epoll_event ev;
                ev.data.fd = clntfd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clntfd,&ev);
                pthread_mutex_lock(&mtx);
                fds.insert(clntfd);
                pthread_mutex_unlock(&mtx);
            }else{
                //cout<<"recv a connection request"<<endl;
                clntfd = evlist[i].data.fd;
                recvlen = recv(clntfd,recvbuffer,4096,0);
                if(recvlen == 0){
                    cout<<"client close a connection"<<endl;
                    pthread_mutex_lock(&mtx);
                    fds.erase(clntfd);
                    pthread_mutex_unlock(&mtx); 
                    close(clntfd);                   
                } else if(recvlen < 10){
                    if(!issending){
                        issending = true;
                        int op = recvbuffer[0]-'0';
                        cout<<"recv a test case request ,case : "<<op<<endl;
                        thread sendingthread(sending_thread,op);
                        sendingthread.detach();
                    } 
                }
            }
        }
    }
}


int main(){
    signal(SIGPIPE,ingnore_sigpipe);
    std::ifstream conf("config.conf");
    std::string title;

    getline(conf,title);
    int pktnum , prdmin , prdmax , isbig;
    
    while(conf >> pktnum){
        conf >> prdmin >> prdmax >> isbig;
        SendConfig conf = { pktnum , prdmin , prdmax , isbig};
        modes.push_back(conf);
    }
    init_socket();
    std::thread ser(service_thread);
    ser.detach();  

    // optional, the test can be invoked by client packet or enter the test num. 
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