#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <mutex>

#include "getTime.cc"

#define PORT 9000
int server_sockfd = -1;
int client_sockfd[3] = {-1, -1, -1};
struct sockaddr_in client_addr;
int data_num = 1000;
std::mutex lock;
std::string last_send_packet;

int count = -1;
int packetSendNumber = 0;

void handlerThread(int sockfd, int flag) {
    std::string packet;
    if (flag > 0) {
        lock.lock();
        send(sockfd, last_send_packet.c_str(), last_send_packet.size(), MSG_NOSIGNAL);
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    while (1) {
        lock.lock();
        std::cout << "do while" << std::endl;
        if (count > flag || packetSendNumber == data_num) {
            //std::cout << last_send_packet << std::endl;
            // last_send_packet = packet;
            lock.unlock();
            break;
        }
        //memset(sendBuffer, 0, sizeof(sendBuffer));
        packet = "PacketNumber:" + std::to_string(packetSendNumber++) + " sendTime:" + std::to_string(getMicros());
        last_send_packet = packet;
        //std::cout << time.c_str() << " " << time.size() << std::endl;
        //std::cout << time << std::endl;
        send(sockfd, packet.c_str(), packet.size(), MSG_NOSIGNAL);
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char const *argv[])
{
	if (argc == 2)
		data_num = atoi(argv[1]);
    std::cout << data_num << std::endl;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        printf("socket can not be initialized!\n");
        exit(-1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        close(server_sockfd);
        printf("bind error!\n");
        exit(-1);
    }

    if(listen(server_sockfd, 5) < 0)
    {
        printf("listen error!\n");
        exit(-1);
    }

    socklen_t sockLen = sizeof(client_addr);

    std::thread t[3];

    while(1) {
        int fd;
        if ((fd = accept(
        server_sockfd, (struct sockaddr*) &client_addr, &sockLen)) < 0) {
            printf("listen error!\n");
            exit(-1);
        }
        count++;
        //std::cout << last_send_packet << std::endl;
        //send(fd, last_send_packet.c_str(), last_send_packet.size(), 0);
        client_sockfd[count] = fd;
        t[count] = std::thread(handlerThread, fd, count);
        if (count == 2)
            break;
        t[count].detach();
    }
    t[2].join();
    // for (int i = 0; i < data_num; ++i) {
    //     //memset(sendBuffer, 0, sizeof(sendBuffer));
    //     std::string time = "PacketNumber:" + std::to_string(i) + " time:" + std::to_string(getMicros());
    //     std::cout << time.c_str() << " " << time.size() << std::endl;
    //     send(client_sockfd, time.c_str(), time.size(), 0);
    //     //usleep(5000);
    // }
    //std::this_thread::sleep_for(std::chrono::seconds(30));
    close(client_sockfd[0]);
    close(client_sockfd[1]);
    close(client_sockfd[2]);
    close(server_sockfd);
    return 0;
}