#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <unordered_set>

//manager可以考虑写成unordered_map<uint32_t, sockaddr_in>保存在conn_manager中?
//尽量用unordered_map,哈希查找，速度>>红黑树?
//可以考虑manager选用LRU结构进行管理，清理LRU的末端？
class ConnidManager{
public:
    static uint32_t getConnID();
    static void delConnID(uint32_t connID);
    static int local_conn_id; //client only
private:
    static uint32_t getNewRandom32();
    static std::unordered_set<uint32_t> ConnID_Manager;    
};

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

//仅作为测试?
// int main(int argc, char const *argv[])
// {
//     std::cout << getConnID() << " " << getConnID() << std::endl;
//     return 0;
// }
