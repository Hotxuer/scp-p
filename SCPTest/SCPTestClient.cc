#include "../TCPTest/getTime.cc"
#include <thread>
#include <unordered_map>
#include <fstream>

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

void runTest1(int& sockfd) {
    int pktNum;
    testData.clear();
    while (pktNum != pktNum1) {
        //简单写一个接收消息的循环即可
        //假设接受的内容为data，是char数组，recv前先memset data
        //recvpkt
        //pktNum++
        //假设recv的内容长度是len
        if (len)
            testData[data] = getMicros();
        
    }
    recordTestResult(1);
}

void runTest2(int& sockfd) {
    int pktNum;
    testData.clear();
    while (pktNum != pktNum1) {
        //简单写一个接收消息的循环即可
        //假设接受的内容为data，是char数组，recv前先memset data
        //recvpkt
        //pktNum++
        //假设recv的内容长度是len
        if (len)
            testData[data] = getMicros();
        if (pktNum == pktNum2 / 2) {
            //把sockfd bind的port给改了
            //reset();
        }
    }
    recordTestResult(2);
}

void runTest3(int& sockfd) {
    bool reset1 = false;
    auto start = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    int pktNum;
    testData.clear();
    while (pktNum != pktNum1) {
        //简单写一个接收消息的循环即可
        //假设接受的内容为data，是char数组，recv前先memset data
        //recvpkt
        //pktNum++
        //假设recv的内容长度是len
        if (len)
            testData[data] = getMicros();
        if (pktNum == pktNum3 / 2) {
            //把sockfd bind的port给改了
            //reset();
            start = std::chrono::system_clock::now();
            reset1 = true;
        }
        if (reset1) {
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            if (elapsed_seconds.count() > 3.0) {
                reset1 = false;
                //把sockfd bind的port改了
                //reset();
            }
        }
        
    }
    recordTestResult(3);
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
    int sockfd;
    //to initialize sockfd 
    std::cout << "start test1" <<std::endl;
    runTest1(sockfd);
    //to close sockfd
    std::cout << "finish test1, start test2 in 5 seconds" <<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    //to initialized sockfd
    runTest2(sockfd);
    //to close sockfd
    std::cout << "finish test2, start test3 in 5 seconds" <<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    //to initialized sockfd
    runTest3(sockfd);
    //to close sockfd
    std::cout << "finish test3" << std::endl;
    return 0;
}
