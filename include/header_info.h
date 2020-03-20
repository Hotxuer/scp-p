#ifndef HEADER_INFO
#define HEADER_INFO

#include "common_lib.h"

/**
   * \struct iphead
   * \brief the ip layer packet header
   * 
   * \see https://en.wikipedia.org/wiki/IPv4
  */
struct iphead{
    /**
    * ip_hl contains the size of the IPv4 header, 
    * it has 4 bits that specify the number of 32-bit words in the header. 
    * The minimum value for this field is 5, and the maximum is 15
    * 
    * ip_version is the four-bit version field. For IPv4, this is always equal to 4.
    */
    unsigned char ip_hl:4, ip_version:4;

    /**
    * ip_tos means Type of Service. The 8bits defined as: PPP DTRC0
    * 
    * PPP is used to define the priority of the packet.
    * 000-routine, 001-priority, 010-immediate, 011-flash
    * 100-falsh override, 101-CRI/TIC/ECP, 110-Internetwork control, 111-network control
    * 
    * D means delay, 0-normal, 1-minimize delay
    * T means handling capacity, 0-normal, 1-maximize handling capacity
    * R means realibility, 0-normal, 1-maximize
    * M means transmission cost, 0-normal, 1-minimize
    * 0 means last bit is always set to 0
    */
    unsigned char ip_tos;

    /**
    * This 16-bit field defines the entire packet size in bytes, including header and data. 
    * The minimum size is 20 bytes (header without data) and the maximum is 65,535 bytes.
    */
    uint16_t ip_len;

    /**
    * This field is an identification field and is primarily 
    * used for uniquely identifying the group of fragments of a single IP datagram. 
    */
    uint16_t ip_id;

    /**
    * This field contains 3 bits Flags and 13 bits Fragment Offset
    * 
    * Flags is a three-bit field follows and is used to control or identify fragments. They are (in order, from most significant to least significant):
    * bit 0: Reserved; must be zero.
    * bit 1: Don't Fragment (DF)
    * bit 2: More Fragments (MF)
    * 
    * Fragment Offset is measured in units of eight-byte blocks. 
    * It is 13 bits long and specifies the offset of a particular fragment relative 
    * to the beginning of the original unfragmented IP datagram.
    */
    uint16_t ip_off;

    /**
    * An eight-bit time to live field helps 
    * prevent datagrams from persisting on an internet
    */
    uint8_t ip_ttl;

    /**
    * This field defines the protocol used in the data portion of the IP datagram.
    */
    uint8_t ip_pro;

    /**
    * The 16-bit IPv4 header checksum field is used for error-checking of the header.
    */
    uint16_t ip_sum;

    /**
    * This field is the IPv4 address of the sender of the packet.
    */
    uint32_t ip_src;

    /**
    * This field is the IPv4 address of the receiver of the packet.
    */
    uint32_t ip_dst;
};


/**
   * \struct tcphead
   * \brief the tcp packet header struct
   * 
   * \see https://en.wikipedia.org/wiki/Transmission_Control_Protocol
  */
struct tcphead{
    /**
    * This 16bits field Identifies the sending port.
    */
    uint16_t tcp_sport;

    /**
    * This 16 bits field Identifies the receiving port.
    */
    uint16_t tcp_dport;
    
    /**
    * This 32 bits field has a dual role:
    * If the SYN flag is set (1), then this is the initial sequence number. 
    * The sequence number of the actual first data byte and the acknowledged number in 
    * the corresponding ACK are then this sequence number plus 1.
    * 
    * If the SYN flag is clear (0), then this is the accumulated sequence number of the 
    * first data byte of this segment for the current session.
    */
    uint32_t tcp_seq;

    /**
    * If the ACK flag is set then the value of this field is the 
    * next sequence number that the sender of the ACK is expecting.
    */
    uint32_t tcp_ack;

    /**
    * tcp_offsSpecifies the size of the TCP header in 32-bit words.
    * The minimum size header is 5 words and the maximum is 15 words 
    * thus giving the minimum size of 20 bytes and maximum of 60 bytes,
    * allowing for up to 40 bytes of options in the header.
    * 
    * tcp_len is 3+1 bits. 
    * The first 3bits is for future use and should be set to zero.
    * The last bit is part of the tcp_flag
    */
    unsigned char tcp_off:4, tcp_len:4;

    /**
    * Contains 9 1-bit flags (control bits).
    * \see https://en.wikipedia.org/wiki/Transmission_Control_Protocol#TCP_segment_structure
    */
    uint8_t tcp_flag;

    /**
    * This field is the size of the receive window, 
    * which specifies the number of window size units that the sender of this segment is currently willing to receive.
    */
    uint16_t tcp_win;

    /**
    * The 16-bit tcp_sum field is used for error-checking of the header, the Payload and a Pseudo-Header. 
    * The Pseudo-Header consists of the Source IP Address, the Destination IP Address, 
    * the protocol number for the TCP-Protocol (0x0006) and the length of the TCP-Headers including Payload (in Bytes).
    */
    uint16_t tcp_sum;

    /**
    * If the tcp_urp flag is set, then this 16-bit field is an offset from the sequence number indicating the last urgent data byte.
    */
    uint16_t tcp_urp;
};


/**
   * \struct udphead
   * \brief the udp packet header struct
   * 
   * \see https://en.wikipedia.org/wiki/User_Datagram_Protocol
  */
struct udphead{
    /**
    * This field identifies the sender's port, when used, and should be assumed to be the port to reply to if needed. 
    * If not used, it should be zero
    */
    uint16_t udp_sport;

    /**
    * This field identifies the receiver's port and is required. 
    */
    uint16_t udp_dport;

    /**
    * This field specifies the length in bytes of the UDP header and UDP data. 
    * The minimum length is 8 bytes, the length of the header. 
    * The field size sets a theoretical limit of 65,535 bytes (8 byte header + 65,527 bytes of data) for a UDP datagram.
    */
    uint16_t udp_len;

    /**
    * The checksum field may be used for error-checking of the header and data.
    */
    uint16_t udp_sum;
};

/**
   * \struct psdhead
   * \brief contains the field used to calculate the checksum in ip header
  */
struct psdhead{
    /**
    * the source address
    */
    unsigned int saddr;

    /**
    * the destination address
    */
    unsigned int daddr;

    /**
    * the empty fields
    */
    unsigned char mbz;

    /**
    * the protocol type
    */
    unsigned char ptcl; //协议类型

    /**
    * the tcp length
    */
    unsigned short tcpl;
};


/**
   * \struct TcpHeadInfo
   * \brief contains the field used to generate tcp packet
  */
struct TcpHeaderInfo {
    /**
    * the destination ip address
    */
    in_addr_t dest_ip;

    /**
    * the source port number
    */
    uint16_t src_port;

    /**
    * the destination port number
    */
    uint16_t dest_port;

    /**
    * the sequential number
    */
    uint32_t seq;

    /**
    * the ack number
    */
    uint32_t ack;

    /**
    * it is used to set the tcp_flag.
    * 0-shake_hand
    * 1-shake_hand_ack
    * 2-data
    */
    int type;
};

typedef struct iphead iphead;
typedef struct tcphead tcphead;
typedef struct TcpHeaderInfo headerinfo;

/**
   * \struct scphead
   * \brief define the scp-p protocol header
  */
struct scphead{
    /**
    * type filed
    * 0: ack, 1: connect/close, 2: data, 3: keep-alive
    * 
    * pktnum and ack have a dual role.
    * If type = 3, all the fields set to 0.
    * If type = 0 or 2, the fields means the sequential number and ack number
    * If type = 1
    * pktnum = 0x7fff, ack = 0 means 1st shakehand
    * pktnum = 0, ack = 0x7fff means 2nd shakehand
    * pktnum = 0x7fff, ack = 0x7fff means 3rd shakehand
    * pktnum = 0, ack = 0 means close
    */
    uint32_t type:2,pktnum:15,ack:15;

    /**
    * connid means the only connection id allocated by ConnIdManager
    */  
    uint32_t connid;
};
#endif