#include "include/conn_manager.h"

//-------------------------------------------------------
// ConnManager static
//-------------------------------------------------------
std::map<uint32_t,FakeConnection*> ConnManager::conn;
std::map<addr_port,uint32_t> ConnManager::addr_pool;
struct sockaddr_in ConnManager::local_addr;
int ConnManager::local_send_fd = 0;
int ConnManager::local_recv_fd = 0;
bool ConnManager::tcp_enable = false;
bool ConnManager::isserver = true;
uint64_t ConnManager::min_rtt = 20;


//-------------------------------------------------------
// ConnManager implementation
//-------------------------------------------------------
FakeConnection* ConnManager::get_conn(uint32_t connid){
    if(conn.find(connid) == conn.end()) return nullptr;
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
    if (addr_pool.find(addr) != addr_pool.end()) {
        return addr_pool[addr] == connid;
    }
    addr_pool[addr] = connid;
    return true;
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

//��init��ʱ��ʹ����̣߳�ÿ��min_rttִ��
void ConnManager::resend_and_clear() {
    LOG(INFO) << "resend and clear thread start";
    while (true) {
        //close��ʱ����min_rtt����Ϊ0
        if (!min_rtt)
            return;
       // printf("resend and clear!\n");
        //ÿ��һ��min_rtt����һ�Σ��ش���ʱ�䲻����ô׼ȷ�����ǿ��Լ����̵߳Ĵ������л�����
        std::this_thread::sleep_for(std::chrono::milliseconds(min_rtt));
        std::vector<FakeConnection*> conns = get_all_connections();
        for (FakeConnection *conn : conns) {
            if (!conn->is_established()) {
                LOG(INFO) << "this link not established, resend 2nd handshake.";
                reply_syn(conn->get_addr(), conn->connection_id);
                continue;
            }
            uint64_t now = getMillis();   
            uint64_t active = conn->get_last_acitve_time();
            int gap = now > active ? now - active : 0;
            //60����û��active���ж�Ϊdead connection��������
            if (ConnManager::isserver && gap >= 3600000) {
                LOG(INFO) << "clear dead connection, id: " << conn->get_conn_id();
                del_addr(conn->get_addr());
                del_conn(conn->get_conn_id());
            } else /*if (ConnManager::isserver && gap > 240000)
            {
                conn->pkt_send(nullptr, 0);
            } else*/ {
                conn->resend_lock.lock();
                for (auto i = conn->resend_map.cbegin(); i != conn->resend_map.cend(); ++i) {
                    if (i->second <= now) {
                        if (!conn->lock_buffer(i->first))
                            continue;
                        conn->pkt_resend(i->first);
                        conn->unlock_buffer(i->first);
                    }
                }
                conn->resend_lock.unlock();
                // for (size_t i = 0; i < BUF_NUM; ++i) {
                //     if (conn->nextSendtime[i] && conn->nextSendtime[i] <= now) {
                //         if (!conn->lock_buffer(i))
                //             continue;
                //         conn->pkt_resend(i);
                //         conn->unlock_buffer(i);
                //     }
                // }
            }
        }
    }
}

//--------------------------------------------------------
// FakeConnection implementation
//--------------------------------------------------------

FakeConnection::FakeConnection(addr_port addr_pt):remote_ip_port(addr_pt){
    remote_sin.sin_family = AF_INET;
    remote_sin.sin_addr.s_addr = addr_pt.sin;
    remote_sin.sin_port = addr_pt.port;
    pkt_in_buf = 0;
    using_tcp = 1;
    now_rtt = 20;
    rtt_cal_rate = 0.2;
    last_active_time = getMillis();
};

FakeConnection::FakeConnection(addr_port addr_pt, AES_KEY enc_key, AES_KEY dec_key):remote_ip_port(addr_pt), aes_enc_key(enc_key), aes_dec_key(dec_key){
    remote_sin.sin_family = AF_INET;
    remote_sin.sin_addr.s_addr = addr_pt.sin;
    remote_sin.sin_port = addr_pt.port;
    pkt_in_buf = 0;
    using_tcp = 1;
    now_rtt = 20;
    rtt_cal_rate = 0.2;
    last_active_time = getMillis();
    key_set_ok();
};


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
// return -1 : shakehand packet
// return 2 : data packet
// return 3: keep alive packet
// return 4: echo keep alive packet
int FakeConnection::on_pkt_recv(void* buf,size_t len,addr_port srcaddr){ // udp modify ok
    // scp packet come in.
    // myack += len; //TCP ack
    scphead* scp = (scphead*) buf;

    last_active_time = getMillis();

    if(!(srcaddr == remote_ip_port)){ // client ip change.
        ConnManager::del_addr(remote_ip_port);
        remote_ip_port = srcaddr;
        ConnManager::add_addr(srcaddr,connection_id);   
    }

    if(scp->type == 0){ // ack
        uint16_t pkt_ack = scp->ack % BUF_NUM;
        LOG(INFO) << "recv an ack packet.";
        //printf("ack_coming.\n");
        if(!buf_used.test(pkt_ack)){
            LOG(INFO) << "this is a redundent ack.";
            //redundent ack
            return 0;
        }
        // need to lock the buffer first
        while(!lock_buffer(pkt_ack)){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        uint64_t this_rtt = getMillsDiff(sendtime[pkt_ack]);
        if (this_rtt == 0)
            this_rtt = 1;
        // if(now_rtt == 0) now_rtt = this_rtt;
        now_rtt = now_rtt*(1-rtt_cal_rate) + this_rtt*rtt_cal_rate;
        ConnManager::min_rtt = std::min(now_rtt, ConnManager::min_rtt);
        //printf("release buffer.\n");
        buf_used.reset(pkt_ack);
        buflen[pkt_ack] = 0;
        pkt_in_buf--;
        resend_lock.lock();
        resend_map.erase(pkt_ack);
        //nextSendtime[pkt_ack] = 0;
        resend_lock.unlock();
        //printf("pkt_in_buf: %ld\n",pkt_in_buf);
        unlock_buffer(pkt_ack);
        return 1;
    }else if(scp->type == 1) { 
        //���������յ��������ֱ���
        if (scp->ack == 0x7fff && scp->pktnum == 0x7fff)
            ConnManager::get_conn(scp->connid)->establish_ok();
        return -1;
    }else if(scp->type == 2) { // data ,sendback_ack
        //�������˿���û��established���յ�data���ģ���ظ�һ���������ֱ���
        // if (!ConnManager::get_conn(scp->connid)->is_established()) {
        //     reply_syn(remote_ip_port, scp->conn_id);
        //     return -5;
        // }

        uint16_t pkt_seq = scp->pktnum;
        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen = 0; 

        unsigned char ack_buf[64];
        
        if(ConnManager::tcp_enable){
            generate_tcp_packet(ack_buf,hdrlen,h);
            generate_udp_packet(ack_buf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
        }

        generate_scp_packet(ack_buf + hdrlen,0,0,pkt_seq,connection_id);
        //uint32_t sendsz = sizeof(tcphead) + sizeof(udphead) +sizeof(scphead) ;
        uint32_t sendsz = hdrlen + sizeof(scphead) ;
        sendto(ConnManager::local_send_fd,ack_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        return 2;
    }else if(scp->type == 3) { 
        //recv keep-alive packet, do echo
        if (scp->connid == 0 && scp->pktnum == 0) {
            headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
            size_t hdrlen = 0; 

            unsigned char heart_beat_buf[30];
        
            if(ConnManager::tcp_enable){
                generate_tcp_packet(heart_beat_buf,hdrlen,h);
                generate_udp_packet(heart_beat_buf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
            }
            generate_scp_packet(heart_beat_buf + hdrlen,3,0x7fff,0x7fff,connection_id);
        
            //uint32_t sendsz = sizeof(tcphead)+ sizeof(udphead) + sizeof(scphead);
            uint32_t sendsz = sizeof(scphead) + hdrlen ;
            sendto(ConnManager::local_send_fd,heart_beat_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
            return 3;
        } else if (scp->connid == 0x7fff && scp->pktnum == 0x7fff) {
            return 4;
        }
    }
}

//resend logic may need to change.
// void packet_resend_thread(FakeConnection* fc, size_t bufnum){
//     while(true){
//         uint64_t resend_wait = fc->now_rtt;
//         std::this_thread::sleep_for(std::chrono::milliseconds(resend_wait));
//         if(!fc->lock_buffer(bufnum)){
//             break;
//         }        
//         if(fc->pkt_resend(bufnum) == 0){
//             fc->unlock_buffer(bufnum);
//             break; 
//         }
//         fc->unlock_buffer(bufnum);
//     }
// }

// add scpheader / tcpheader.
ssize_t FakeConnection::pkt_send(const void* buffer,size_t len){ // modify ok
    if (len == -1) {
        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen = 0; 
        unsigned char heart_beat_buf[30];
        if(ConnManager::tcp_enable){
            generate_tcp_packet(heart_beat_buf,hdrlen,h);
            generate_udp_packet(heart_beat_buf+hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
            myseq += sizeof(scphead) + sizeof(udphead);
        }
        
        generate_scp_packet(heart_beat_buf + hdrlen,1,0,0,connection_id);
        
        uint32_t sendsz = hdrlen + sizeof(scphead);
        int sz = sendto(ConnManager::local_send_fd,heart_beat_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        LOG_IF(ERROR, sz < 0) << "send to error, errno: " << errno;
        return 0;
    }
    if(!is_establish) {
        LOG(WARNING) << "pkt send when not established.";
        // printf("not established .\n");
        return 0;
    }
    if (buffer == nullptr) {
        //send heart beat
        headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
        size_t hdrlen = 0; 
        unsigned char heart_beat_buf[30];
        if(ConnManager::tcp_enable){
            generate_tcp_packet(heart_beat_buf,hdrlen,h);
            generate_udp_packet(heart_beat_buf+hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
            myseq += sizeof(scphead) + sizeof(udphead);
        }
        
        generate_scp_packet(heart_beat_buf + hdrlen,3,0,0,connection_id);
        
        uint32_t sendsz = hdrlen + sizeof(scphead);
        int sz = sendto(ConnManager::local_send_fd,heart_beat_buf,sendsz,0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
        LOG_IF(ERROR, sz < 0) << "send to error, errno: " << errno;
        return 0;
    }
    // select a buffer.
    int bufnum = get_used_num();
    if(bufnum == -1){
        LOG(ERROR) << "buffer is full, cannot send data.";
        //printf("buffer is full.\n");
        return 0;
    }
    // encrypto the buffer
    if (!encrypto_pkg((unsigned char*)buffer, len)) {
        LOG(WARNING) << "No aes key set, encrypto failed.";
    }
    // select a buffer and copy the packet to the buffer
    uint16_t tot_len;
    if(ConnManager::tcp_enable){
        tot_len =  sizeof(scphead) + sizeof(tcphead) + sizeof(udphead) + len;
    }else{
        tot_len = sizeof(scphead) + len;
    }
    memcpy(buf[bufnum] + tot_len - len,buffer,len);
    buflen[bufnum] = tot_len;

    // generate tcp_scp_packet
    //uint16_t iplen = (uint16_t) (len+sizeof(scphead));
    headerinfo h= {remote_ip_port.sin,ConnManager::get_local_port(),remote_ip_port.port,myseq,myack,2};
    size_t hdrlen = 0;
    uint16_t tbufnum = (uint16_t) bufnum;
    if(ConnManager::tcp_enable){
        generate_tcp_packet((unsigned char*)buf[bufnum], hdrlen , h);
        myseq += sizeof(scphead) + len + sizeof(udphead);//TCP seq
        generate_udp_packet((unsigned char*)buf[bufnum] + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead) + len);
    }
    generate_scp_packet((unsigned char*)buf[bufnum] + hdrlen,2,tbufnum,0,connection_id);
    ssize_t sendbytes = 0;

#ifdef CLNTMODE
    //for(int i = 0;i < 2; i++){
        sendbytes = sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin));
    //}
#else
    //printf("before send\n");    

    //if(bufnum % 10)
    sendbytes = sendto(ConnManager::local_send_fd,buf[bufnum],buflen[bufnum],0,(struct sockaddr*) &remote_sin,sizeof(remote_sin)); 
    sendtime[bufnum] = getMillis();
    resend_lock.lock();
    // nextSendtime[bufnum] = sendtime[bufnum] + now_rtt;
    resend_map[bufnum] = sendtime[bufnum] + now_rtt;
    resend_lock.unlock();
    //std::cout << nextSendtime << std::endl;
    //std::cout << "rtt" << now_rtt << std::endl;
    //printf("after send.\n");
    // std::thread resend_thread(packet_resend_thread,this,bufnum);
    // resend_thread.detach();
    LOG_IF(ERROR, sendbytes < 0) << "send to error, errno: " << errno;
    return sendbytes;
#endif
    
}

void FakeConnection::set_RTT(uint64_t rtt) {
    this->now_rtt = rtt;
}

uint64_t FakeConnection::get_RTT() {
    return this->now_rtt;
}

void FakeConnection::set_RTT_cal_rate(double rate) {
    this->rtt_cal_rate = rate;
}

void FakeConnection::set_aes_key(AES_KEY enc_key, AES_KEY dec_key) {
    this->aes_enc_key = enc_key;
    this->aes_dec_key = dec_key;
    key_set_ok();
}

AES_KEY FakeConnection::get_aes_enc_key() {
    return this->aes_enc_key;
}

AES_KEY FakeConnection::get_aes_dec_key() {
    return this->aes_dec_key;
}

int FakeConnection::encrypto_pkg(unsigned char* buffer, const size_t len) {
    if (!is_key_set()) return 0;
    int tem = 0;
    AES_KEY key = get_aes_enc_key();
    // loop encryption, the length of each loop is AES_BLOCK_SIZE
    while (tem < len) {
        AES_encrypt(buffer+tem, buffer+tem, &key);
        tem += AES_BLOCK_SIZE;
    }
    return -1;
}

int FakeConnection::decrypto_pkg(unsigned char* buffer, const size_t len) {
    if (!is_key_set()) return 0;
    int tem = 0;
    AES_KEY key = get_aes_dec_key();
    // loop decryption, the length of each loop is AES_BLOCK_SIZE
    while (tem < len) {
        AES_decrypt(buffer+tem, buffer+tem, &key);
        tem += AES_BLOCK_SIZE;
    }
    return -1;
}

size_t FakeConnection::pkt_resend(size_t bufnum){
    if(!is_establish) {
        resend_map[bufnum] += 1.5*now_rtt;
        //nextSendtime[bufnum] += now_rtt;
        LOG(WARNING) << "not established, resend failed!";
        //printf("not establish,resend failed.\n");
        return 1;
    } 
    if(!buf_used.test(bufnum)){
        return 0;
    }
    resend_map[bufnum] += 1.5*now_rtt;
    LOG(INFO) << "resend packet to connection id: " << connection_id << " from buffer " << bufnum;
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

int reply_syn(addr_port src,uint32_t& conn_id){
    int ret = 1;
    if(ConnManager::exist_addr(src)){
        //printf("exist address.\n");
        //printf("conn_id : %d.\n",conn_id);
        conn_id = ConnManager::get_connid(src);
        //printf("conn_id : %d.\n",conn_id);
    }else if(conn_id == 0 || !ConnManager::exist_conn(conn_id)){ // a new request or the connid not exist
        // generate aes key
        AES_KEY enc_key, dec_key;
        char user_key[AES_BLOCK_SIZE];
        memcpy(user_key, "zheshiopensslexq", 16);
        AES_set_encrypt_key((const unsigned char *)user_key, AES_BLOCK_SIZE * 8, &enc_key);
        AES_set_decrypt_key((const unsigned char *)user_key, AES_BLOCK_SIZE * 8, &dec_key);
        conn_id = ConnidManager::getConnID();
        ConnManager::add_conn(conn_id,new FakeConnection(src, enc_key, dec_key));
        ConnManager::get_conn(conn_id)->set_conn_id(conn_id);
        //ConnManager::get_conn(conn_id)->establish_ok();
        ConnManager::get_conn(conn_id)->update_para(0,1);
        ConnManager::add_addr(src,conn_id);
        
        // std::thread thr(wait_reply_syn_ack, src, conn_id);
        // thr.detach();

        ret = 2;
    }
    unsigned char ackbuf[40];
    headerinfo h = {src.sin,ConnManager::get_local_port(),src.port,0,1,1};
    size_t hdrlen = 0;

    if(ConnManager::tcp_enable){
        generate_tcp_packet(ackbuf,hdrlen,h);
        generate_udp_packet(ackbuf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
    }
    generate_scp_packet(ackbuf + hdrlen,1,0,0x7fff,conn_id);

    sockaddr_in rmt_sock_addr;
    
    rmt_sock_addr.sin_family = AF_INET;
    rmt_sock_addr.sin_addr.s_addr = src.sin;
    rmt_sock_addr.sin_port = src.port;

    //printf("port : %d\n",src.port);
    int sz = sendto(ConnManager::local_send_fd,ackbuf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&rmt_sock_addr,sizeof(rmt_sock_addr));
    //printf("send sz : %d\n",sz);
    if(sz == -1){
        int err = errno;
        //printf("errno %d.\n",err);
        LOG(ERROR) << "reply syn error, errno: " << err;
    }
    return ret;  
}

int reply_syn_ack(addr_port src, uint32_t& conn_id) {
    //int ret = 3;
    unsigned char ackbuf[40];

    headerinfo h = {src.sin,ConnManager::get_local_port(),src.port,0,1,2};
    size_t hdrlen = 0;

    if(ConnManager::tcp_enable){
        generate_tcp_packet(ackbuf,hdrlen,h);
        generate_udp_packet(ackbuf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
        //myseq += sizeof(scphead) + sizeof(udphead);
    }
    generate_scp_packet(ackbuf + hdrlen,1,0x7fff,0x7fff,conn_id);

    sockaddr_in rmt_sock_addr;
    
    rmt_sock_addr.sin_family = AF_INET;
    rmt_sock_addr.sin_addr.s_addr = src.sin;
    rmt_sock_addr.sin_port = src.port;

    //printf("port : %d\n",src.port);
    int sz = sendto(ConnManager::local_send_fd,ackbuf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&rmt_sock_addr,sizeof(rmt_sock_addr));
    //printf("send sz : %d\n",sz);
    if(sz == -1){
        int err = errno;
        LOG(ERROR) << "reply syn-ack error, errno: " << err;
    }
    return 3;  
}

int reply_close(addr_port src, uint32_t& conn_id) {
    unsigned char closebuf[40];

    headerinfo h = {src.sin,ConnManager::get_local_port(),src.port,0,0,2};
    size_t hdrlen = 0;
    
    if(ConnManager::tcp_enable){
        generate_tcp_packet(closebuf,hdrlen,h);
        generate_udp_packet(closebuf + hdrlen,h.src_port,h.dest_port,hdrlen,sizeof(scphead));
        //myseq += sizeof(scphead) + sizeof(udphead);
    }
    generate_scp_packet(closebuf + hdrlen,1,0,0,conn_id);

    sockaddr_in rmt_sock_addr;
    
    rmt_sock_addr.sin_family = AF_INET;
    rmt_sock_addr.sin_addr.s_addr = src.sin;
    rmt_sock_addr.sin_port = src.port;

    //printf("port : %d\n",src.port);
    int sz = sendto(ConnManager::local_send_fd,closebuf,hdrlen+sizeof(scphead),0,(struct sockaddr *)&rmt_sock_addr,sizeof(rmt_sock_addr));
    //printf("send sz : %d\n",sz);
    if(sz == -1){
        int err = errno;
        LOG(ERROR) << "reply syn-ack error, errno: " << err;
        return sz;
    }
    return 0;  
}