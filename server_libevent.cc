#include "scp_interface.h"
#include <cstring>
#include <event2/event.h>
#include <event2/event_struct.h>


#define LOCAL_PORT_USED 17001   
//#define REMOTE_PORT_USED 17001

#define LOCAL_ADDR "202.120.38.131"
#define REMOTE_ADDR "202.120.38.100"


struct event_args{
    char recvbuf[2048];
    char sendbuf[256];
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

    //headerinfo h;
    //stat = parse_frame(ev_args->recvbuf + 14,n-14,this_conn_id,ev_args->isserver);
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
}

void service_thread(bool isserver){
    evutil_socket_t listen_raw_socket = (evutil_socket_t) ConnManager::local_recv_fd;
    struct event_base* base;
    struct event *listener_event;

    event_args evarg;
    evarg.isserver = isserver;
    base = event_base_new();
    listener_event = event_new(base,listen_raw_socket,EV_READ|EV_PERSIST,handle_event,&evarg);

    event_add(listener_event,NULL);
    event_base_dispatch(base);
}

int main(int argc,char** argv){

    int ret = init_rawsocket(false, true);
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    /*
    int ret = init_rawsocket(my_bpf_code,sizeof(my_bpf_code)/sizeof(struct sock_filter));
    if(ret) printf("init_rawsocket error.");
    */

    //connect(htons(LOCAL_ADDR),htons(REMOTE_ADDR));
    std::thread ser(service_thread,true);
    ser.detach();
    sleep(20);// wait for the connection;
    char str[10] = "hello_scp";
    std::vector<FakeConnection*> v = ConnManager::get_all_connections();
    size_t sendsz;
    for(auto i : v){
        sendsz = scp_send(str,9,i);
        printf("send:--%ld\n",sendsz);
    }
    //size_t sendsz = send(str,9,inet_addr(REMOTE_ADDR));
    
    sleep(10000);
}