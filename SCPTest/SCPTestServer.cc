#include "../TCPTest/getTime.cc"
#include <thread>

int pktNum1, pktNum2, pktNum3;



int main(int argc, char const *argv[])
{
    pktNum1 = pktNum2 = pktNum3 = 1000;
    if (argc >= 2)
        pktNum1 = atoi(argv[1]);
    if (argc >= 3)
        pktNum2 = atoi(argv[2]);
    if (argc >= 4)
        pktNum3 = atoi(argv[3]);

    //initialize server
    //server会发送pkt,pkt内容如下
    std::string packet = "PacketNumber:" + std::to_string(count++) + " sendTime:" + std::to_string(getMicros());
    //连接成功后，用一个count记录发送成功的报文数
    if (count == pktNum1) {
        count = 0;
        std::cout << "finish test1" << std::endl;
    }
    //等待第二次测试的连接
    //做的事情仍旧是发送pkt即可，中途会有一次client的reset但是不影响server
    if (count == pktNum2) {
        count = 0;
        std::cout << "finish test2" << std::endl;
    }
    //等待第三次测试的连接
    //做的事情仍旧是发送pkt即可，中途会有两次client的reset但是不影响server
    if (count == pktNum2) {
        count = 0;
        std::cout << "finish test3" << std::endl;
    }
    //close server 
    return 0;
}
