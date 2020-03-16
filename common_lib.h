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

uint64_t getSeconds();

uint64_t getSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count(); 
}


// Get time stamp in milliseconds.
uint64_t getMillis()
{
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    return ms; 
}

// Get time stamp in microseconds.
inline uint64_t getMicros()
{
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    return us; 
}

uint64_t getMillsDiff(uint64_t past) {
    uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    return now - past;
}

uint64_t getMicrosDiff(uint64_t past) {
    uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    return now - past;
}
#endif // !COMMON_LIB_H