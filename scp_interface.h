#include "packet_generator.h"
#include "frame_parser.h"

int scp_bind(in_addr_t localip , uint16_t port){
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = localip;
    local_addr.sin_port = htons(port);

    ConnManager::set_local_addr(local_addr);

    int sendfd = ConnManager::local_send_fd;
    // tcpdump -dd 'tcp[2:2] == 17001 and tcp[tcpflags] & tcp-rst == 0'
    struct sock_filter my_bpf_code[] = {
        { 0x28, 0, 0, 0x0000000c },
        { 0x15, 0, 10, 0x00000800 },
        { 0x30, 0, 0, 0x00000017 },
        { 0x15, 0, 8, 0x00000006 },
        { 0x28, 0, 0, 0x00000014 },
        { 0x45, 6, 0, 0x00001fff },
        { 0xb1, 0, 0, 0x0000000e },
        { 0x48, 0, 0, 0x00000010 },
        { 0x15, 0, 3, 0x00004269 },
        { 0x50, 0, 0, 0x0000001b },
        { 0x45, 1, 0, 0x00000004 },
        { 0x6, 0, 0, 0x00040000 },
        { 0x6, 0, 0, 0x00000000 },
    };

    my_bpf_code[8] = { 0x15, 0, 3, port & 0xffffffff};

    struct sock_fprog filter;
    filter.filter = my_bpf_code;
    filter.len = sizeof(my_bpf_code)/sizeof(struct sock_filter);

    if (setsockopt(ConnManager::local_recv_fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) < 0) {
        //perror("setsockopt fail\n"); 
        printf("setsockopt fail failed\n");
        return -1;  
    }

    if(sendfd != 0){
        return bind(sendfd,(sockaddr*) &local_addr,sizeof(local_addr));
    }
}


// init a raw socket
int init_rawsocket(){
    if(ConnManager::local_send_fd != 0 || ConnManager::local_send_fd != 0){
        // This method should only be active once.
        return -1;
    }
    // initial recv_fd,收到的为以太网帧，帧头长度为18字节
    int recv_rawsockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (recv_rawsockfd < 0 ) {
        //perror("socket fail\n");
        printf("recv socket initial failed\n");
        return -1;
    }else{
        ConnManager::local_recv_fd = recv_rawsockfd;
    }

    //initial send_fd
    int send_rawsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(send_rawsockfd < 0){
        printf("send socket initial failed\n");
        return -1;
    }else{
        ConnManager::local_send_fd = send_rawsockfd;
    }
    /*
    int one = 1;
    
    if(setsockopt(send_rawsockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0){   //定义套接字不添加IP首部，代码中手工添加
        printf("setsockopt failed!\n");
        return -1;
    }
    */

    return 0;
}


int scp_connect(in_addr_t remote_ip,uint16_t remote_port){
    uint32_t local_id = ConnidManager::local_conn_id;
    printf("local id : %d.\n",local_id);
    if(local_id != 0){ // reconnect
        ConnManager::get_conn(local_id)->establish_rst();
    }
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(remote_port);
    server_addr.sin_addr.s_addr = remote_ip;
    
    addr_port remote_ad_pt = {remote_ip , htons(17001)};
    // add to the local_conn_manager.
    // ConnManager::add_conn(remote_ad_pt,new FakeConnection(false,remote_ad_pt));

    // send syn
    headerinfo h= {remote_ip,ConnManager::get_local_port(),remote_ad_pt.port,0,0,0};
    size_t hdrlen;
    unsigned char tmp_send_buf[45];
    generate_tcp_packet(tmp_send_buf,hdrlen,h);
    generate_scp_packet(tmp_send_buf+hdrlen,2,0,0,ConnidManager::local_conn_id);
    sendto(ConnManager::local_send_fd,tmp_send_buf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&server_addr,sizeof(server_addr));

    printf("send syn ok\n"); 

    uint32_t sleep_time = 10000,max_resend = 5;

    usleep(sleep_time);

    while(ConnidManager::local_conn_id == local_id && !ConnManager::get_conn(local_id)->is_established() && max_resend--){
        sendto(ConnManager::local_send_fd,tmp_send_buf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&server_addr,sizeof(server_addr));
        sleep_time *= 2;
        usleep(sleep_time);
    }

    local_id = ConnidManager::local_conn_id;
    if(ConnManager::get_conn(local_id)->is_established()){
        return 1;
    }else{
        return 0;
    }
    //return 0;
}


size_t scp_send(const char* buf,size_t len,FakeConnection* fc){
    //addr_port ta = {rmtaddr,htons(17001)};
    if(!fc) return 0;
    return fc->pkt_send(buf,len);
}