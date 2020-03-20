#ifndef PKT_GEN
#define PKT_GEN

#include "header_info.h"

/**
   * \brief generate the tcp packet with the giving payload and header info
   *
   * \param buf tcp payload buffer 
   * \param len len is a value-result argument to record the total packet header length. 
   *            Before the call, it should be initialized to a special value, such as 0 or Ethernet frame length.
   *            After the call, it will add the tcp header length.
   * \param info some important message to construct the tcp header
   * \see struct TcpTeaderInfo
   * \return 0 means success
   */
int generate_tcp_packet(unsigned char* buf, size_t & len,headerinfo info);

/**
   * \brief generate the scp packet with the giving payload and info
   *
   * \see struct scphead;
   * \param buf scp payload buffer 
   * \param type scp packet type. 
   *             0: ack type, 1: connect/close type, type 2: data type, 3: keep alive type 
   * \param pktnum the scp packet sequential number
   * \param ack the scp ack number
   * \param conn_id the scp connection id
   * \return 0 means success
   */
int generate_scp_packet(unsigned char* buf,uint8_t type,uint16_t pktnum,uint16_t ack,uint32_t conn_id);

/**
   * \brief generate the ucp packet with the giving payload and info
   *
   * \param buf scp payload buffer 
   * \param srcport the source port number
   * \param destport the destination port number
   * \param len len is a value-result argument to record the total packet header length. 
   *            Before the call, it should be initialized to a special value, such as tcp header length.
   *            After the call, it will add the udp header length.
   * \param payload_len the udp payload length
   * \return 0 means success
   */
int generate_udp_packet(unsigned char* buf, uint16_t srcport , uint16_t destport,size_t & len,size_t payload_len);


#endif
