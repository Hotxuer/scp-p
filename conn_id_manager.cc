#include "include/conn_id_manager.h"

std::unordered_set<uint32_t> ConnidManager::ConnID_Manager;
int ConnidManager::local_conn_id = 0;

uint32_t ConnidManager::getNewRandom32() {
    unsigned int x;
    x = rand() & 0xff;
    x |= (rand() & 0xff) << 8;
    x |= (rand() & 0xff) << 16;
    x |= (rand() & 0xff) << 24;
    return x;
}

uint32_t ConnidManager::getConnID() {
    uint32_t x = 0;
    while (x == 0 || ConnID_Manager.find(x) != ConnID_Manager.end()) {
        x = getNewRandom32();
    }
    return x;
}

void ConnidManager::delConnID(uint32_t connID) {
    if (ConnID_Manager.find(connID) == ConnID_Manager.end()) {
        //print error
        return;
    } else {
        ConnID_Manager.erase(connID);
    }
}