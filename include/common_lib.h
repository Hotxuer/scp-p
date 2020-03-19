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
uint64_t getMillis();
uint64_t getMicros();
uint64_t getMillsDiff(uint64_t past);
uint64_t getMicrosDiff(uint64_t past);

#endif // !COMMON_LIB_H