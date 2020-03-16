#ifndef CONN_MNG
#define CONN_MNG
#include "packet_generator.h"
#define BUF_SZ 2048
#define BUF_NUM 1024
#include "conn_id_manager.h"


class FakeConnection;

struct addr_port{
    in_addr_t sin;
    uint16_t port;
    bool operator < (const addr_port& a1) const{
        return sin < a1.sin;
    }
    bool operator == (const addr_port& a1) const{
      return (sin == a1.sin && port == a1.port);
    }
};

class ConnManager{
public:
    static FakeConnection* get_conn(uint32_t connid);
    static int add_conn(uint32_t connid,FakeConnection* scp_conn);
    static bool exist_conn(uint32_t connid);
    static size_t del_conn(uint32_t connid);
    static void set_local_addr(const sockaddr_in local);
    static void set_local_ip(const in_addr_t local_ip);
    static void set_local_port(const uint16_t local_port); 
    static in_addr_t get_local_ip();
    static uint16_t get_local_port();
    static int local_send_fd;
    static int local_recv_fd;
    static std::vector<FakeConnection*> get_all_connections();

    static bool exist_addr(addr_port addr);
    static bool add_addr(addr_port addr ,uint32_t connid);
    static bool del_addr(addr_port addr);
    static uint32_t get_connid(addr_port addr);
    static int clear_dead_conn();
    //static int local_connid; // client only
private:
    static std::map<uint32_t,FakeConnection*> conn;
    static struct sockaddr_in local_addr;
    static std::map<addr_port,uint32_t> addr_pool;

};

std::map<uint32_t,FakeConnection*> ConnManager::conn;
std::map<addr_port,uint32_t> ConnManager::addr_pool;
struct sockaddr_in ConnManager::local_addr;
int ConnManager::local_send_fd = 0;
int ConnManager::local_recv_fd = 0;


void packet_resend_thread(FakeConnection* fc, size_t bufnum);

class FakeConnection{
    friend void packet_resend_thread(FakeConnection* fc, size_t bufnum);
public:
    FakeConnection() = default;
    //FakeConnection(bool isser):isserver(isser){};
    FakeConnection(bool isser,addr_port addr_pt):isserver(isser),remote_ip_port(addr_pt){
        remote_sin.sin_family = AF_INET;
        remote_sin.sin_addr.s_addr = addr_pt.sin;
        remote_sin.sin_port = addr_pt.port;
        pkt_in_buf = 0;
        using_tcp = 1;
        now_rtt = 20000;
        last_active_time = getSeconds();
    };

    int on_pkt_recv(void* buf,size_t len,addr_port srcaddr);
    size_t pkt_send(const void* buf,size_t len);
    size_t pkt_resend(size_t bufnum);

    ~FakeConnection() = default;
    void establish_ok(){ is_establish = true; };
    void establish_rst(){ is_establish = false; };
    bool is_established(){ return is_establish; };
    void update_para(uint32_t seq,uint32_t ack){ myseq = seq; myack = ack; };

    bool lock_buffer(size_t bufnum);
    void unlock_buffer(size_t bufnum);

    void set_conn_id(uint32_t connid);

    uint32_t get_conn_id();
    //std::mutex buf_mutex[BUF_NUM];
    uint64_t get_last_acitve_time();
    addr_port get_addr();

    //void set_tcp_enable(bool using){ using_tcp = using; };
    bool is_tcp_enable(){ return using_tcp; }
private:
    // -- protocal options --
    bool using_tcp;
    uint64_t sendtime[BUF_NUM];
    uint32_t now_rtt;

    // -- tcp info --
    uint32_t connection_id;
    bool isserver;
    uint32_t myseq,myack;
    bool is_establish;
    addr_port remote_ip_port;
    sockaddr_in remote_sin;

    uint64_t last_active_time;

    // -- buffer management --
    char buf[BUF_NUM][BUF_SZ]; 
    uint16_t buflen[BUF_NUM];
    
    // get the bufnum can be used.
    int get_used_num();

    size_t pkt_in_buf , pvt; // pvt is the first free buffer we need to find
    std::bitset<BUF_NUM> buf_used;

    // a lock used for retransmit
    std::bitset<BUF_NUM> buf_lock;
};


//-------------------------------------------------------
// ConnManager implementation
//-------------------------------------------------------
FakeConnection* ConnManager::get_conn(uint32_t connid){
    if(conn.find(connid) == conn.end()) return NULL;
    return conn[connid];
}

int ConnManager::add_conn(uint32_t connid,FakeConnection* scp_conn){
    conn[connid] = scp_conn;
    return 0;
}

bool ConnManager::exist_conn(uint32_t connid){
    if(conn.find(connid) != conn.end()) return true;
    else return false;
}

size_t ConnManager::del_conn(uint32_t connid){
    if(conn.find(connid) != conn.end()){
        delete conn[connid];
    }
    return conn.erase(connid);
    //return 0;
}

void ConnManager::set_local_addr(const sockaddr_in local){
    local_addr = local;
}

void ConnManager::set_local_ip(const in_addr_t local_ip){
    local_addr.sin_addr.s_addr = local_ip;
}

void ConnManager::set_local_port(const uint16_t local_port){
    local_addr.sin_port = local_port;
}

in_addr_t ConnManager::get_local_ip(){
    return local_addr.sin_addr.s_addr;
}

uint16_t ConnManager::get_local_port(){
    return local_addr.sin_port;
}
std::vector<FakeConnection*> ConnManager::get_all_connections(){
    std::vector<FakeConnection*> v;
    auto b_inserter = back_inserter(v);
    for(auto i = conn.cbegin();i != conn.cend();i++){
        *b_inserter = i->second;
    }
    return v;
}

bool ConnManager::exist_addr(addr_port addr){
    if(addr_pool.find(addr) != addr_pool.end()){
        return true;
    }
    return false;
}

bool ConnManager::add_addr(addr_port addr,uint32_t connid){
    addr_pool[addr] = connid;
    //return addr_pool.insert(addr).second;
}

bool ConnManager::del_addr(addr_port addr){
    return addr_pool.erase(addr);
}

uint32_t ConnManager::get_connid(addr_port addr){
    if(addr_pool.find(addr) == addr_pool.end()){
        return 0;
    }else{
        return addr_pool[addr];
    }
}

int ConnManager::clear_dead_conn() {
    std::vector<FakeConnection*> conns = get_all_connections();
    uint64_t now = getSeconds();    
    
    for (FakeConnection *conn : conns) {
        uint64_t active = conn->get_last_acitve_time();
        int gap = now > active ? now - active : 0;
        //60分钟没有active则判定为dead connection，做清理
        if (gap > 3600) {
            del_addr(conn->get_addr());
            del_conn(conn->get_conn_id());
        } else if (gap > 300)
        {
            conn->pkt_send(nullptr, 0);
        }
    }
}
//--------------------------------------------------------
// FakeConnection implementation
//--------------------------------------------------------

bool FakeConnection::lock_buffer(size_t bufnum){
    if(buf_lock.test(bufnum)){
        return false;
    }
    buf_lock.set(bufnum);
    return true;
}

void FakeConnection::unlock_buffer(size_t bufnum){
    buf_lock.reset(bufnum);
}

// return 0 : redundent ack
// return 1 : legal ack
// return -1 : reset request
// return 2 : data packet
int FakeConnection::on_pkt_recv(void* buf,size_t len,addr_port srcaddr){ // udp modify ok
    // scp packet come in.
    // myack += len; //TCP ack
    scphead* scp = (scphead*) buf;

    last_active_time = getSeconds();

    if(!(srcaddr == remote_ip_port)){ // client ip change.
        ConnManager::del_addr(remote_ip_port);
        remote_ip_port = srcaddr;
        ConnManager::add_addr(srcaddr,connection_id);   
    }

    if(scp->type == 0){ // ack
        uint16_t pkt_ack = scp->ack;
        printf("ack_coming.\n");
        if(!buf_used.test(pkt_ack)){
            //redundent ack
            return 0;
        }
        // need to lock the buffer first
        while(!lock_buffer(pkt_ack)){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        uint64_t this_rtt = getMicrosDiff(sendtime[pkt_ack]);
        // if(now_rtt == 0) now_rtt = this_rtt;
        now_rtt = now_rtt*0.8 + this_rtt*0.2;
        //printf("release buffer.\n");
        buf_used.reset(pkt_ack);
        buflen[pkt_ack] = 0;
        pkt_in_buf--;
        //printf("pkt_in_buf: %ld\n",pkt_in_buf);
        unlock_buffer(pkt_ack);
        return 1;
    }else if(scp->type == 1) { // reset
        return -1;
    }else if(scp->type == 2) { // data ,sendback_ack
        uint16_t pkt_seq = scp->pktnum;

        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen = 0; 

        unsigned char ack_buf[30];
        if(using_tcp)
            generate_tcp_packet(ack_buf,hdrlen,h);
        generate_udp_packet(ack_buf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
        generate_scp_packet(ack_buf + hdrlen,0,0,pkt_seq,connection_id);
        uint32_t sendsz = sizeof(tcphead) + sizeof(udphead) +sizeof(scphead) ;
        sendto(ConnManager::local_send_fd,ack_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        return 2;
    }else if(scp->type == 3) { //heart beat, send heart beat back
        //if server recv heart beat, it is an echo from client
        //do nothing, it only use to update the last_active_time
        if (isserver)
            return 3;
        
        //if client recv heart beat, echo the packet
        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen = 0; 

        unsigned char heart_beat_buf[30];
        if(using_tcp)
            generate_tcp_packet(heart_beat_buf,hdrlen,h);
        generate_udp_packet(heart_beat_buf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
        generate_scp_packet(heart_beat_buf + hdrlen,3,0,0,connection_id);
        uint32_t sendsz = sizeof(tcphead)+ sizeof(scphead);
        sendto(ConnManager::local_send_fd,heart_beat_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        return 3;
    }
}

// resend logic may need to change.
void packet_resend_thread(FakeConnection* fc, size_t bufnum){
    int maxresend = 5;
    uint64_t resend_wait = fc->now_rtt*2;
    while(maxresend--){
        std::this_thread::sleep_for(std::chrono::milliseconds(resend_wait));
        resend_wait *= 2;
        if(!fc->lock_buffer(bufnum)){
            break;
        }
        
        if(fc->pkt_resend(bufnum) == 0){
            fc->unlock_buffer(bufnum);
            break; 
        }
        fc->unlock_buffer(bufnum);
    }
}

// add scpheader / tcpheader.
size_t FakeConnection::pkt_send(const void* buffer,size_t len){ // modify ok
    if(!is_establish) {
        printf("not established .\n");
        return 0;
    }
    if (buffer == nullptr) {
        //send heart beat
        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen = 0; 
        unsigned char heart_beat_buf[30];
        if(using_tcp)
            generate_tcp_packet(heart_beat_buf,hdrlen,h);
        generate_udp_packet(heart_beat_buf+hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
        generate_scp_packet(heart_beat_buf + hdrlen,3,0,0,connection_id);
        uint32_t sendsz = sizeof(tcphead) + sizeof(udphead) +sizeof(scphead);
        sendto(ConnManager::local_send_fd,heart_beat_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        return 0;
    }
    // select a buffer.
    int bufnum = get_used_num();
    if(bufnum == -1){
        printf("buffer is full.\n");
        return 0;
    }
    // select a buffer and copy the packet to the buffer
    uint16_t tot_len =  sizeof(scphead) + sizeof(tcphead) + sizeof(udphead) + len;
    memcpy(buf[bufnum] + tot_len - len,buffer,len);
    buflen[bufnum] = tot_len;

    // generate tcp_scp_packet
    //uint16_t iplen = (uint16_t) (len+sizeof(scphead));
    headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
    size_t hdrlen = 0;
    if(using_tcp)
        generate_tcp_packet((unsigned char*)buf[bufnum], hdrlen , h);
    myseq += sizeof(scphead) + len;//TCP seq
    uint16_t tbufnum = (uint16_t) bufnum;
    generate_udp_packet((unsigned char*)buf[bufnum] + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead) + len);
    generate_scp_packet((unsigned char*)buf[bufnum] + hdrlen,2,tbufnum,0,connection_id);
    size_t sendbytes = 0;

#ifdef CLNTMODE
    //for(int i = 0;i < 2; i++){
        sendbytes = sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
    //}
#else
    printf("before send");    

    //if(bufnum % 10)
    for(int i = 0;i < 2; i++){
        sendbytes = sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
    } 
    sendtime[bufnum] = getMicros();
    //printf("after send.\n");
    std::thread resend_thread(packet_resend_thread,this,bufnum);
    resend_thread.detach();
    return sendbytes;
#endif
    
}

size_t FakeConnection::pkt_resend(size_t bufnum){
    if(!is_establish) {
        printf("not establish,resend failed.\n");
    } 
    if(!buf_used.test(bufnum)){
        return 0;
    }
    return sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
}

int FakeConnection::get_used_num(){
    if(pkt_in_buf >= BUF_NUM){ 
        return -1;
    }
    size_t tpvt = pvt;
    while(pvt != (tpvt + BUF_NUM- 1)%BUF_NUM){
        if(!buf_used.test(pvt) && !buf_lock.test(pvt)){
            buf_used.set(pvt);
            pvt = (pvt+1) % BUF_NUM;
            pkt_in_buf++;
            //buf_mutex[pvt].unlock();
            if(pvt == 0) return BUF_NUM - 1;
            return pvt - 1;
        }else{
            pvt = (pvt+1) % BUF_NUM;
        }
    }
    return -1;
}

void FakeConnection::set_conn_id(uint32_t connid){
    connection_id = connid;
}

addr_port FakeConnection::get_addr() {
    return remote_ip_port;
}

uint32_t FakeConnection::get_conn_id(){
    return connection_id;
}

uint64_t FakeConnection::get_last_acitve_time() {
    return last_active_time;
}
#endif
