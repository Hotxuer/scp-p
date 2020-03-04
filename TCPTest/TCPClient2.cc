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
#include <fstream>
#include <unordered_map>
#include "getTime.cc"

#define SERVER_PORT 9000
#define SERVER_IP "202.120.38.131"
struct sockaddr_in server_addr;
char data[1024];

int doConnect() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket initialized error!\n");
        exit(-1);
    }

	memset(&server_addr,0,sizeof(server_addr)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_addr.s_addr=inet_addr(SERVER_IP);
	server_addr.sin_port=htons(SERVER_PORT);
	
	if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))<0)
	{
		printf("connect error!");
		close(sockfd);
		exit(-1);
	}
    return sockfd;
}

std::unordered_map<std::string, uint64_t> statistic_map;

int main(int argc, char const *argv[])
{
	int data_num = 1000;
	if (argc == 2)
		data_num = atoi(argv[1]);
    int reconnect_num = data_num / 2;

    int sockfd = doConnect();

	printf("connected to server\n");

	int len, count = 0;

	while(1)
	{
		memset(data, 0, 1024);
		len = recv(sockfd, data, 1024, 0);
		//data[len] = '\0';
        if (len)
		    statistic_map[std::string(data)] = getMicros();
		count++;
        //std::cout << data << std::endl;
		if (count == reconnect_num)
            break;
	}
	
	close(sockfd);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    sockfd = doConnect();
    std::cout << "do reconnect" << std::endl;
    while(len)
	{
        //std::cout << "len :" << len << std::endl;
		memset(data, 0, 1024);
		len = recv(sockfd, data, 1024, 0);
		//data[len] = '\0';
        if(len)
		    statistic_map[std::string(data)] = getMicros();
        //std::cout << data << std::endl;
	}

    close(sockfd);

	std::ofstream file;
	file.open("TCPTest2.txt");
	for (auto i = statistic_map.begin(); i != statistic_map.end(); ++i) {
		std::string past = (i->first).substr((i->first).find_last_of(":") + 1);
        //std::cout << i->first << "   " << past << std::endl;
		uint64_t time = (i->second) - std::stoull(past);
		file << i->first << " recvTime:" << i->second;
		file << " timeCost:" << time << '\n'; 
	}
	file.close();

    return 0;
}
