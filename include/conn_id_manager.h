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



//仅作为测试?
// int main(int argc, char const *argv[])
// {
//     std::cout << getConnID() << " " << getConnID() << std::endl;
//     return 0;
// }
