#ifndef CONN_MNG
#define CONN_MNG
#include "packet_generator.h"
#define BUF_SZ 2048
#define BUF_NUM 64
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
    static bool add_addr(addr_port addr);
    static bool del_addr(addr_port addr);

    //static int local_connid; // client only
private:
    static std::map<uint32_t,FakeConnection*> conn;
    static struct sockaddr_in local_addr;
    static std::set<addr_port> addr_pool;

};

std::map<uint32_t,FakeConnection*> ConnManager::conn;
std::set<addr_port> ConnManager::addr_pool;
struct sockaddr_in ConnManager::local_addr;
int ConnManager::local_send_fd = 0;
int ConnManager::local_recv_fd = 0;


class FakeConnection{
public:
    FakeConnection() = default;
    //FakeConnection(bool isser):isserver(isser){};
    FakeConnection(bool isser,addr_port addr_pt):isserver(isser),remote_ip_port(addr_pt){
        remote_sin.sin_family = AF_INET;
        remote_sin.sin_addr.s_addr = addr_pt.sin;
        remote_sin.sin_port = addr_pt.port;
    };
    int on_pkt_recv(void* buf,size_t len,addr_port srcaddr);
    size_t pkt_send(void* buf,size_t len);
    size_t pkt_resend(size_t bufnum);

    ~FakeConnection() = default;
    void establish_ok(){ is_establish = true; };
    bool is_established(){ return is_establish; };
    void update_para(uint32_t seq,uint32_t ack){ myseq = seq; myack = ack; };

    bool lock_buffer(size_t bufnum);
    void unlock_buffer(size_t bufnum);

    void set_conn_id(uint32_t connid);

private:
    // -- tcp info --
    uint32_t connection_id;
    bool isserver;
    uint32_t myseq,myack;
    bool is_establish;
    addr_port remote_ip_port;
    sockaddr_in remote_sin;

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

bool ConnManager::add_addr(addr_port addr){
    return addr_pool.insert(addr).second;
}

bool ConnManager::del_addr(addr_port addr){
    return addr_pool.erase(addr);
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

int FakeConnection::on_pkt_recv(void* buf,size_t len,addr_port srcaddr){ // modify ok
    // scp packet come in.
    // myack += len; //TCP ack
    scphead* scp = (scphead*) buf;
    if(scp->type == 0){ // ack
        uint16_t pkt_ack = scp->ack;
        if(!buf_used.test(pkt_ack)){
            //redundent ack
            return 0;
        }
        // need to lock the buffer first
        while(!lock_buffer(pkt_ack)){
            buf_used.reset(pkt_ack);
            buflen[pkt_ack] = 0;
            unlock_buffer(pkt_ack);
        }
        return 1;
    }else if(scp->type == 1) { // reset
        return -1;
    }else if(scp->type == 2) { // data ,sendback_ack
        uint16_t pkt_seq = scp->pktnum;

        if(!(srcaddr == remote_ip_port)){ // client ip change.
            remote_ip_port = srcaddr;
        }

        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen; 

        unsigned char ack_buf[30];
        generate_tcp_packet(ack_buf,hdrlen,h);
        generate_scp_packet(ack_buf + hdrlen,0,0,pkt_seq,connection_id);
        uint32_t sendsz = sizeof(tcphead)+ sizeof(scphead);
        sendto(ConnManager::local_send_fd,ack_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        return 2;
    }
}

// resend logic may need to change.
void packet_resend_thread(FakeConnection* fc, size_t bufnum){
    int maxresend = 5;
    while(maxresend--){
        usleep(400000);
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
size_t FakeConnection::pkt_send(void* buffer,size_t len){ // modify ok
    if(!is_establish) {
        printf("not established .\n");
        return 0;
    }
    // select a buffer.
    int bufnum = get_used_num();
    if(bufnum == -1){
        printf("buffer is full.\n");
        return 0;
    }
    // select a buffer and copy the packet to the buffer
    uint16_t tot_len =  sizeof(scphead) + sizeof(tcphead)  + len;
    memcpy(buf[bufnum] + tot_len - len,buffer,len);
    buflen[bufnum] = tot_len;

    // generate tcp_scp_packet
    //uint16_t iplen = (uint16_t) (len+sizeof(scphead));
    headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
    size_t hdrlen;
    generate_tcp_packet((unsigned char*)buf[bufnum], hdrlen , h);
    myseq += sizeof(scphead) + len;//TCP seq
    uint16_t tbufnum = (uint16_t) bufnum;
    generate_scp_packet((unsigned char*)buf[bufnum] + hdrlen,2,tbufnum,0,connection_id);
    size_t sendbytes = 0;
#ifdef CLNTMODE
    //for(int i = 0;i < 2; i++){
        sendbytes = sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
    //}
#else
    printf("before send\n");
    for(int i = 0;i < 2; i++){
        sendbytes = sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
    } 
    printf("after send.\n");
    // TODO: Create a thread for resend.  
    std::thread resend_thread(packet_resend_thread,this,bufnum);
    resend_thread.detach();
    return sendbytes;
#endif
    
}

size_t FakeConnection::pkt_resend(size_t bufnum){
    if(!buf_used.test(bufnum)){
        return 0;
    }
    return sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
}

int FakeConnection::get_used_num(){
    if(pkt_in_buf >= BUF_NUM - 2){ // at least 2 empty
        return -1;
    }
    size_t tpvt = pvt;
    while(pvt != (tpvt + BUF_NUM- 1)%BUF_NUM){
        if(!buf_used.test(pvt) && !buf_lock.test(pvt)){
            buf_used.set(pvt);
            pvt++;
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
#endif