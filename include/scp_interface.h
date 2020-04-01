#include "packet_generator.h"
#include "frame_parser.h"

/**
   * \brief bind a local port to socket fd
   *
   * \param localip local network ip
   * \param port bind port number
   * \return -1 means failed, 0 means success
   */
int scp_bind(in_addr_t localip , uint16_t port);

/**
   * \brief init a socket and create the resend and clear thread
   *
   * \param tcpenable choose to add the Fake TCPheader or not
   * \param isserver choose to init in server or in client device
   * \return -1 means failed, 0 means success
   */
int init_rawsocket(bool tcpenable, bool isserver);

/**
   * \brief init a socket and create the resend and clear thread with Fake Tcpheader mode
   *
   * \param isserver choose to init in server or in client device
   * \return -1 means failed, 0 means success
   */
int init_rawsocket(bool isserver);

/**
   * \brief connect to the server, only use in the client device
   *
   * \param remote_ip the connect server ip
   * \param remote_port the connect server port
   * \return 0 means failed, 1 means success
   */
int scp_connect(in_addr_t remote_ip,uint16_t remote_port);

/**
   * \brief send scp data
   *
   * \param buf the scp data content
   * \param len send data length
   * \see ConnManager::get_all_connections()
   * \see ConnManager::get_conn()
   * \see ConnidManager::local_conn_id
   * \param fc the fakeConnection pointer
   *           in server, use ConnManager::get_all_connections() to get all the connected client
   *           or can use ConnManager::get_conn(uint32_t connid) to get the target client
   *           in client, use ConnManager::get_conn(ConnidManager::local_conn_id) to get the target server
   * \return if success, return the actual send length, 0 means the fc is not exist, -1 means failed 
   */
ssize_t scp_send(const char* buf,size_t len,FakeConnection* fc);

/**
   * \brief send scp keep-alive packet
   *
   * \param fc the fakeConnection pointer
   *           in server, use ConnManager::get_all_connections() to get all the connected client
   *           or can use ConnManager::get_conn(uint32_t connid) to get the target client
   *           in client, use ConnManager::get_conn(ConnidManager::local_conn_id) to get the target server
   * \return if success, return 0, -1 means failed 
   */
int scp_send_keep_alive(FakeConnection* fc);

/**
   * \brief get the now calculated RTT
   *
   * \param fc the fakeConnection pointer
   *           in server, use ConnManager::get_all_connections() to get all the connected client
   *           or can use ConnManager::get_conn(uint32_t connid) to get the target client
   *           in client, use ConnManager::get_conn(ConnidManager::local_conn_id) to get the target server
   * \return now calculated RTT of fc
   */
uint64_t get_RTT(FakeConnection* fc);

/**
   * \brief set the RTT of the fc
   *
   * \param rtt the set value
   * \param fc the fakeConnection pointer
   *           in server, use ConnManager::get_all_connections() to get all the connected client
   *           or can use ConnManager::get_conn(uint32_t connid) to get the target client
   *           in client, use ConnManager::get_conn(ConnidManager::local_conn_id) to get the target server 
   */
void set_RTT(uint64_t rtt, FakeConnection* fc);

/**
   * \brief set the auto calculate rate of rtt
   *
   * \param rate the calculate rate, rtt_new = rate*now_rtt + (1-rate)*rtt_old
   *             if rate = 0, rtt is a const
   * \param fc the fakeConnection pointer
   *           in server, use ConnManager::get_all_connections() to get all the connected client
   *           or can use ConnManager::get_conn(uint32_t connid) to get the target client
   *           in client, use ConnManager::get_conn(ConnidManager::local_conn_id) to get the target server 
   */
void auto_cal_RTT(double rate, FakeConnection* fc);

/**
   * \brief close the scp socket, clear the FakeConnection and the working thread, only can be called by client
   *
   * \return 0 means success, -1 means failed
   */
int scp_close();

/**
   * \brief close the scp socket, clear the FakeConnection and the working thread, only can be called by server
   *
   * \param fc the fakeConnection pointer
   * \return 0 means success, -1 means failed
   */
int scp_close_all();

/**
   * \brief close the scp socket, clear the FakeConnection and the working thread, only can be called by server
   *
   * \param fc the fakeConnection pointer
   * \return 0 means success, -1 means failed
   */
int scp_close(FakeConnection *fc);

/**
   * \brief init the glog module
   *
   * \param name name of the running project
   * \param dest destination of log file
   * \return 0 means success, -1 means failed
   */
int init_glog(const char* name, const char* dest);