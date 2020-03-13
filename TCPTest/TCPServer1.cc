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
#include <netinet/tcp.h>

#include "getTime.cc"

#define PORT 7000
int server_sockfd;
int client_sockfd;
struct sockaddr_in client_addr;

int main(int argc, char const *argv[])
{
    int flag;
    int data_num = 1000;
	if (argc == 2)
		data_num = atoi(argv[1]);
    std::cout << data_num << std::endl;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        printf("socket can not be initialized!\n");
        exit(-1);
    }

    flag = 1;
    setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));

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
    if ((client_sockfd = accept(
        server_sockfd, (struct sockaddr*) &client_addr, &sockLen)) < 0) {
            printf("listen error!\n");
            exit(-1);
    }
    flag = 1;
    setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));

    for (int i = 0; i < data_num; ++i) {
        //memset(sendBuffer, 0, sizeof(sendBuffer));
        std::string time = "PacketNumber:" + std::to_string(i) + " time:" + std::to_string(getMicros());
        //std::cout << time.c_str() << " " << time.size() << std::endl;
        send(client_sockfd, time.c_str(), time.size(), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    close(client_sockfd);
    close(server_sockfd);
    return 0;
}
