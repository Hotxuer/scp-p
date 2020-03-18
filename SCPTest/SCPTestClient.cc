// #include "../TCPTest/getTime.cc"
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
        sleep(3);
        ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
        if(ret == 0){
            printf("connect to server failed.\n");
        }
    }
}

void service_thread(){
    int stat;size_t n;
    char recvbuf[4096];
    uint32_t this_conn_id;
    int recv_packets = 0;
    testData.clear();
    int headlen = ConnManager::tcp_enable ? 70 : 8; 

    addr_port src;
    bool tcpenable = ConnManager::tcp_enable;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrLen = sizeof(fromAddr);
    while(ConnManager::local_recv_fd){
        uint64_t recvTime = getMicros();
         if(tcpenable){
            n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,NULL,NULL);
            stat = parse_frame(recvbuf + 14,n-14,this_conn_id,src);
        }else{
            n = recvfrom(ConnManager::local_recv_fd,recvbuf,4096,0,(struct sockaddr*)&fromAddr,&fromAddrLen);
            src.sin = fromAddr.sin_addr.s_addr;
            src.port = fromAddr.sin_port;
            stat = parse_frame(recvbuf ,n,this_conn_id,src);
        }
        switch (stat){
            case 0:
                printf("recv a data pkt when not established\n");
            case 1:
                printf("a request for exist connnection. \n");
            case 2:
                printf("a request from client.\n");
                break;
            case 3:
                printf("recv a back SYN-ACK from server(not in the server).\n");
                break;
            case 4:
                printf("server recv a pkt with reply-syn-ack.\n");
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
            case 8:
                printf("recv heart beat packet.\n");
            case 9:
                printf("recv close packet.\n");
            case -1:
                printf("recv a illegal frame.\n");
                break;
            default:
                break;
        }
        if(stat == 7){
            std::string data = std::string(recvbuf + headlen,n - headlen);
            if(testData.count(data) == 0){
                testData[data] = recvTime;
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
                printf("finish test!\n");
                sleep(300);
                scp_close();
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

    int ret = init_rawsocket(false, false);
    if(ret) printf("init_rawsocket error.");
    scp_bind(inet_addr(LOCAL_ADDR),LOCAL_PORT_USED);
    std::thread ser(service_thread);
    ser.detach();
    sleep(2);
    ret = scp_connect(inet_addr(REMOTE_ADDR),17001);
    if(ret == 0){
        printf("connect to server failed.\n");
    }
    
    sleep(30000);
    return 0;
}
