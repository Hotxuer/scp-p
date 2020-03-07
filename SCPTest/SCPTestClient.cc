#include "../TCPTest/getTime.cc"
#include <thread>
#include <unordered_map>
#include <fstream>
#include "../scp_interface.h"

#define LOCAL_PORT_USED 17000
#define REMOTE_PORT_USED 17001

#define LOCAL_ADDR "202.120.38.100"
#define REMOTE_ADDR "202.120.38.131"

std::unordered_map<std::string, uint64_t> testData;

int pktNum1, pktNum2, pktNum3;

void recordTestResult(int number) {
    std::ofstream file;
    std::string fileName = std::string("SCPTest") + std::to_string(number) + std::string(".txt");
	file.open(fileName);
	for (auto i = testData.begin(); i != testData.end(); ++i) {
		std::string past = (i->first).substr((i->first).find_last_of(":") + 1);
        //std::cout << i->first << "   " << past << std::endl;
		uint64_t time = (i->second) - std::stoull(past);
		file << i->first << " recvTime:" << i->second;
		file << " timeCost:" << time << '\n'; 
	}
	file.close();
}

void rst_thread(int mode){
    int ret;
    if(mode == 1){
        sleep(7);
        ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
        if(ret == 0){
            printf("connect to server failed.\n");
        }
    }else{
        sleep(7);
        ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
        if(ret == 0){
            printf("connect to server failed.\n");
        }
        sleep(1);
        ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
        if(ret == 0){
            printf("connect to server failed.\n");
        }
    }
}

void service_thread(bool isserver){
    int stat;size_t n;
    char recvbuf[4096];
    addr_port rmt;
    uint32_t this_conn_id;
    int scp_stat;
    int recv_packets = 0;
    testData.clear();
    while(1){
        n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,NULL,NULL);
        stat = parse_frame(recvbuf + 14,n-14,this_conn_id,isserver);
        switch (stat){
            case 1:
            case 2:
                printf("This should not be a client packet.\n");
                break;
            case 3:
                printf("recv a back SYN-ACK from server.\n");
                break;
            case 4:
                printf("recv a reset request.\n");
                break;
            case 5:
                printf("recv a scp redundent ack.\n");
                break;
            case 6:
                printf("recv a scp packet ack.\n");
                break;
            case 7:
                printf("recv a scp data packet.\n");
                break;
            case -1:
                printf("recv a illegal frame.\n");
                break;
            default:
                break;
        }
        if(stat == 7){
            std::string data = std::string(recvbuf + 62,n - 62);
            if(testData.count(data) == 0){
                testData[data] = getMicros();
                recv_packets++;
            }
            if(recv_packets == pktNum1){
                recordTestResult(1);
                testData.clear();
                std::thread rst(rst_thread,1);
                rst.detach();
            }
            if(recv_packets == pktNum1 + pktNum2){
                recordTestResult(2);
                testData.clear();
                std::thread rst2(rst_thread,2);
                rst2.detach();
            }
            if(recv_packets == pktNum1 + pktNum2 + pktNum3){
                recordTestResult(3);
                testData.clear();
            }
        }
        // n = recvfrom(recv_rawsockfd, recvbuf, 1024, 0, NULL, NULL); 
        // stat = parse_frame(recvbuf + 14,n-14);   
    }
}


int main(int argc, char const *argv[])
{
    pktNum1 = pktNum2 = pktNum3 = 1000;
    if (argc >= 2)
        pktNum1 = atoi(argv[1]);
    if (argc >= 3)
        pktNum2 = atoi(argv[2]);
    if (argc >= 4)
        pktNum3 = atoi(argv[3]);

    int ret = init_rawsocket();
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    std::thread ser(service_thread,false);
    ser.detach();
    sleep(2);
    ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
    if(ret == 0){
        printf("connect to server failed.\n");
    }
    
    sleep(10000);
    return 0;
}
