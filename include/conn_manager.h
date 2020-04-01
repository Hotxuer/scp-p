#ifndef CONN_MNG
#define CONN_MNG
#include "packet_generator.h"
#define BUF_SZ 1400
#define BUF_NUM 1024
#include "conn_id_manager.h"

class FakeConnection;

/**
 * \struct addr_port
 * store the ip address and the port
 */ 
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


/**
 * \class ConnManager
 * 
 * manage the scp-p protocol connection
 * every connection has a one and only 32 bits connection ID
 */ 
class ConnManager{
public:
    /**
     * get the fake connection link pointer by connection ID
     * \param connid the 32 bits connection id
     * \see class FakeConnection
     * \return nullptr if the connid is invalid or the corresponding FakeConnection*
     */ 
    static FakeConnection* get_conn(uint32_t connid);

    /**
     * add the connection id to the corresponding FakeConnection pointer
     * \param connid the 32 bits connection id
     * \param scp_conn the corresponding FakeConnection pointer
     * \see class FakeConnection
     * \return 0 if add success
     */ 
    static int add_conn(uint32_t connid,FakeConnection* scp_conn);

    /**
     * inquire if there exist the connection with connection id
     * \param connid the 32 bits connection id
     * \return the inquire result
     */ 
    static bool exist_conn(uint32_t connid);

    /**
     * delete the connection by the id
     * \param connid the 32 bits connection id
     * \return 0 for success
     */ 
    static size_t del_conn(uint32_t connid);

    /**
     * set the local address
     * \param local the sockaddr_in struct represent the loacl address
     */ 
    static void set_local_addr(const sockaddr_in local);

     /**
     * set the local ip address
     * \param local_ip the in_addr_t struct represent the loacl ip address
     */ 
    static void set_local_ip(const in_addr_t local_ip);

     /**
     * set the local port
     * \param local_port the local port binding with SCP-P protocol
     */ 
    static void set_local_port(const uint16_t local_port); 

     /**
     * get the local ip address
     * \return local ip address
     */ 
    static in_addr_t get_local_ip();

    /**
     * get the local ip port
     * \return local port
     */ 
    static uint16_t get_local_port();

    /**
     * if tcp_enable, it represents the raw send socket fd, working on TCP layer, send the ip packet payload
     * else it represents the system udp socket fd
     */ 
    static int local_send_fd;

    /**
     * if tcp_enable, it represents the raw recv fd, working on ethernet layer, recv the ethernet frame payload
     * else it equal to the local_send_fd
     */ 
    static int local_recv_fd;

    /**
     * get all the client connections for server
     * \return a vector that collect all the FakeConnection pointer, each represents a client connection
     */ 
    static std::vector<FakeConnection*> get_all_connections();

    /**
     * inquire if the addr represents a connected client
     * \param addr the inquire addr_port
     * \return the inquire results
     */ 
    static bool exist_addr(addr_port addr);

    /**
     * add the addr_port to the single connid
     * if guarantee each connid will assign a single connection
     * when client change ip or port, the server will update the info
     * 
     * \param addr the corresponding addr_port
     * \param connid the corresponding id
     * \return false if the addr already has another connid; true if add success
     */
    static bool add_addr(addr_port addr ,uint32_t connid);

    /**
     * delete the addr from the addr_pool
     * 
     * \param addr the delete addr_port
     * \return true if delete success, false if there has no such addr in addr_pool 
     */ 
    static bool del_addr(addr_port addr);

    
    /**
     * get the connection id by the given addr_port
     * 
     * \param addr the given addr_port to get the connid
     * \return 0 if there exist on connection with the addr, else return the target connid 
     */ 
    static uint32_t get_connid(addr_port addr);

    /**
     * this function used to do the resend and clear the dead connection。
     * it will be invoke by the created thread in init_rawsocket() function
     */ 
    static void resend_and_clear();

    /**
     * record the minimum RTT among all the connections
     */ 
    static uint64_t min_rtt;
    
    /**
     * record if the ConnManager work on server or client mode
     */ 
    static bool isserver;

    /**
     * record if the scp-p will add fake tcp header
     */ 
    static bool tcp_enable;
private:
    /**
     * record all the FakeConnection pointer by the target conn id
     */ 
    static std::map<uint32_t,FakeConnection*> conn;

    /**
     * record the local addr
     */ 
    static struct sockaddr_in local_addr;

    /**
     * record all the conn id by the target addr_port
     * it used to avoid more than one conn id when 1st handshake packet receive more than once
     */ 
    static std::map<addr_port,uint32_t> addr_pool;

};

//void packet_resend_thread(FakeConnection* fc, size_t bufnum);

/**
 * \class FakeConnection
 * 
 * this class represent the virtual link between client and server
 */ 
class FakeConnection{
    //friend void packet_resend_thread(FakeConnection* fc, size_t bufnum);
public:
    /**
     * default constructor
     */ 
    FakeConnection() = default;
    //FakeConnection(bool isser):isserver(isser){};
    /**
     * constructor with the addr_pt, do some init work
     * \param addr_pt the addr_port of other side of the virtual link device 
     */ 
    FakeConnection(addr_port addr_pt);

    /**
     * do the packet recv work when recv the scp packet
     * 
     * \param buf the received scp packet buffer with scp header
     * \param len length of the buf
     * \param srcaddr the buf sender addr_port
     * \return 0 : redundent ack, 1 : legal ack, -1 : shakehand packet
               2 : data packet, 3: keep alive packet
               4 : echo keep alive packet
     */ 
    int on_pkt_recv(void* buf,size_t len,addr_port srcaddr);

    /**
     * do the scp-p packet send work
     * 
     * \param buf the send scp packet buffer
     * \param len the intend send length of the buf
     * \return 0 : the scp connection is not established, or the actual send length
     */
    ssize_t pkt_send(const void* buf,size_t len);

    /**
     * do the packet resend work to guarantee the reability of scp-p protocol
     * 
     * \param bufnum the resend buffer number
     * \return 0 if no need to resend, 1 if resend failed or the resend packet length if success
     */ 
    size_t pkt_resend(size_t bufnum);

    /**
     * destructor
     */ 
    ~FakeConnection() = default;

    /**
     * set the connection to be established.
     * the client call it when recv 2nd handshake packet, 
     * while the server call it when recv 3rd handshake packet
     */ 
    void establish_ok(){ is_establish = true; };

    /**
     * set the connection to be not established.
     * it will be called when reconnect
     */ 
    void establish_rst(){ is_establish = false; };

    /**
     * inquire if the connection is established.
     * \return the establish state
     */ 
    bool is_established(){ return is_establish; };

    /**
     * record the seq and ack, there will be used to fill the fake TCP header
     * 
     * \param seq the fake tcp header sequential number
     * \param ack the fake tcp header ack number
     */ 
    void update_para(uint32_t seq,uint32_t ack){ myseq = seq; myack = ack; };

    /**
     * lock the buffer with buffer number. use the lock to avoid reassign when recv ack or resend
     * \param bufnum the target lock buffer number
     * \return true if get the lock, false if not
     */ 
    bool lock_buffer(size_t bufnum);

    /**
     * unlock the lock buffer
     * \param bufnum the target unlock buffer number
     */ 
    void unlock_buffer(size_t bufnum);

    /**
     * set the Fake Connection with the single connid
     * \param connid the target connid to relevant with the virtual link
     */ 
    void set_conn_id(uint32_t connid);

    /**
     * get the connection id corresponding to the virtual link
     * \return the connection id
     */ 
    uint32_t get_conn_id();
    //std::mutex buf_mutex[BUF_NUM];

    /**
     * get the last active time of the virtual link
     * \return the last active time of the virtual link in milliseconds
     */ 
    uint64_t get_last_acitve_time();

    /**
     * get the remote addr_port of the virtual link
     * \return the remote addr_port
     */ 
    addr_port get_addr();

    //void set_tcp_enable(bool using){ using_tcp = using; };

    /**
     * \return if the virtual link send scp-p packet with fake tcp header
     */ 
    bool is_tcp_enable(){ return using_tcp; }

    void set_RTT(uint64_t rtt); /**set the now_rtt*/

    uint64_t get_RTT(); /**get the now_rtt*/

    void set_RTT_cal_rate(double rate); /**set the cal now_rtt rate*/
private:
    // -- protocal options --
    bool using_tcp; /** choose if to use fake tcp header*/ 

    uint64_t sendtime[BUF_NUM];/**record the send time of each buffer*/ 

    std::mutex resend_lock; /**lock of the resend_map*/

    std::unordered_map<int, uint64_t> resend_map; /**record the next resend time with the need resend buffer number*/

    // uint64_t nextSendtime[BUF_NUM]; 

    uint64_t now_rtt; /**record the RTT of the virtual link*/

    double rtt_cal_rate; /**the rtt cal rate*/

    // -- tcp info --
    uint32_t connection_id; /**record the connection id of the virtual link*/
    //bool isserver;
    uint32_t myseq,myack; /**record the seq and ack for the fake tcp header content*/
    
    bool is_establish; /**record if the connection is established*/
    
    addr_port remote_ip_port; /**record the remote addr_port of the virtual link*/
    
    sockaddr_in remote_sin; /**record the remote sockaddr_in of the virtual link*/

    uint64_t last_active_time; /**record the last active time of the virtual link in milliseconds*/

    // -- buffer management --
    char buf[BUF_NUM][BUF_SZ]; /**the buffer of the sender*/ 

    uint16_t buflen[BUF_NUM]; /**the length of each sender buffers*/
    
    // get the bufnum can be used.
    /**
     * get the next buffer number that can be used to send scp-p packet
     * 
     * \return the next free send buffer number
     */ 
    int get_used_num();

    size_t pkt_in_buf; /**the buffer used size*/

    size_t pvt; /**pvt is the first free buffer that need to find*/

    std::bitset<BUF_NUM> buf_used; /**record the used send buffer*/
    // a lock used for retransmit
    std::bitset<BUF_NUM> buf_lock; /**do the buffer lock to mutex use the buffer*/

    friend class ConnManager;
};

/**
 * called when recv 1st handshake packet, only called by server!
 * 
 * \param addr_port the connect request client addr
 * \param conn_id the corresponding conn_id, if not exist, allocate a new id
 * \return 1 if it is a exist connect, 2 if it is a new connect
 */ 
int reply_syn(addr_port src,uint32_t& conn_id);

/**
 * called when recv 2nd handshake packet, only called by client!
 * 
 * \param addr_port the syn ack send server addr
 * \param conn_id the corresponding conn_id
 * \return 3 if success
 */ 
int reply_syn_ack(addr_port src, uint32_t& conn_id);

/**
 * called when server recv data packet after close!
 * 
 * \param addr_port the syn ack send server addr
 * \param conn_id the corresponding conn_id
 * \return 0 if success
 */ 
int reply_close(addr_port src, uint32_t& conn_id);

#endif
