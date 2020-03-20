#ifndef COMMON_LIB_H
#define COMMON_LIB_H

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <cstdio>
#include <linux/filter.h>
#include <cstring>
#include <bitset>
#include <unordered_map>
#include <map>
#include <thread>
#include <vector>
#include <set>
#include <mutex>
#include <chrono>

/**
   * \brief get the timestamp in seconds
   * \return timestamp in seconds
   */
uint64_t getSeconds();


/**
   * \brief get the timestamp in milliseconds
   * \return timestamp in milliseconds
   */
uint64_t getMillis();


/**
   * \brief get the timestamp in microseconds
   * \return timestamp in microseconds
   */
uint64_t getMicros();


/**
   * \brief get the time duration between past and now in milliseconds
   * 
   * \param past timestamp in milliseconds
   * \return time duration between past and now in milliseconds
   */
uint64_t getMillsDiff(uint64_t past);

/**
   * \brief get the time duration between past and now in microseconds
   * 
   * \param past timestamp in microseconds
   * \return time duration between past and now in microseconds
   */
uint64_t getMicrosDiff(uint64_t past);

#endif // !COMMON_LIB_H