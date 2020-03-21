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
size_t scp_send(const char* buf,size_t len,FakeConnection* fc);

/**
   * \brief close the scp socket, clear the FakeConnection and the working thread
   *
   * \return 0 means success, -1 means failed
   */
int scp_close();

/**
   * \brief init the glog module
   *
   * \param name name of the running project
   * \param dest destination of log file
   * \return 0 means success, -1 means failed
   */
int init_glog(const char* name, const char* dest);