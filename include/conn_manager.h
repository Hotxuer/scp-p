#ifndef CONN_MNG
#define CONN_MNG
#include "packet_generator.h"
#define BUF_SZ 1400
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
    static void resend_and_clear();
    static uint64_t min_rtt;
    //static int local_connid; // client only
    static bool isserver;

    static bool tcp_enable;
private:
    static std::map<uint32_t,FakeConnection*> conn;
    static struct sockaddr_in local_addr;
    static std::map<addr_port,uint32_t> addr_pool;

};

//void packet_resend_thread(FakeConnection* fc, size_t bufnum);

class FakeConnection{
    //friend void packet_resend_thread(FakeConnection* fc, size_t bufnum);
public:
    FakeConnection() = default;
    //FakeConnection(bool isser):isserver(isser){};
    FakeConnection(addr_port addr_pt);

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

    std::mutex resend_lock;
    std::unordered_map<int, uint64_t> resend_map;
    // uint64_t nextSendtime[BUF_NUM]; 

    uint64_t now_rtt;

    // -- tcp info --
    uint32_t connection_id;
    //bool isserver;
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

    friend class ConnManager;
};


#endif
