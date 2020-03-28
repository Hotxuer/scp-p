#ifndef FRM_PAR
#define FRM_PAR

#include "header_info.h"
#include "conn_manager.h"

/**
   * \brief parse the frame received by recv thread
   *
   * \param buf the receiver buffer filled by system recvfrom() function 
   * \param len the receiver buffer length
   * \param conn_id the value-result argument. When the function return, it set to the conn_id of the target client
   * \param srcaddr the source address of the receive packet 
   * \return 0: recv a data pkt when not established.
             1: recv a request for exist connnection.
             2: recv a request from client.
             3: recv a back SYN-ACK from server(not in the server).
             4: server recv a pkt with reply-syn-ack.
             5: recv a scp redundent ack.
             6: recv a scp packet ack.
             7: recv a scp data packet.
             8: recv a keep alive packet.
             9: recv a echo keep alive packet.
             10:recv a scp close packet.
             11:recv a echo scp close packet.
            -1: recv an illegal packet.
   */
int parse_frame(char *buf, size_t len, uint32_t &conn_id, addr_port &srcaddr);

/**
   * \brief parse the fake tcp frame received by recv thread, called by parse_frame method
   *
   * \param buf the receiver buffer filled by system recvfrom() function 
   * \param len the receiver buffer length
   * \param conn_id the value-result argument. When the function return, it set to the conn_id of the target client
   * \param srcaddr the source address of the receive packet 
   * \return 0: recv a data pkt when not established.
             1: recv a request for exist connnection.
             2: recv a request from client.
             3: recv a back SYN-ACK from server(not in the server).
             4: server recv a pkt with reply-syn-ack.
             5: recv a scp redundent ack.
             6: recv a scp packet ack.
             7: recv a scp data packet.
             8: recv a keep alive packet.
             9: recv a echo keep alive packet.
             10:recv a scp close packet.
             11:recv a echo scp close packet.
            -1: recv an illegal packet.
   */
int parse_tcp_frame(char* buf, size_t len,uint32_t& conn_id,addr_port& srcaddr);

/**
   * \brief parse the scp frame received by recv thread, called by parse_frame method
   *
   * \param buf the receiver buffer filled by system recvfrom() function 
   * \param len the receiver buffer length
   * \param conn_id the value-result argument. When the function return, it set to the conn_id of the target client
   * \param srcaddr the source address of the receive packet 
   * \return 0: recv a data pkt when not established.
             1: recv a request for exist connnection.
             2: recv a request from client.
             3: recv a back SYN-ACK from server(not in the server).
             4: server recv a pkt with reply-syn-ack.
             5: recv a scp redundent ack.
             6: recv a scp packet ack.
             7: recv a scp data packet.
             8: recv a keep alive packet.
             9: recv a echo keep alive packet.
             10:recv a scp close packet.
             11:recv a echo scp close packet.
            -1: recv an illegal packet.
   */
int parse_scp_frame(char* buf, size_t len,uint32_t& conn_id, addr_port& srcaddr);

#endif
