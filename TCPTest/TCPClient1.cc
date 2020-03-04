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

#define PORT 7000
#define SERVER_PORT 7000
#define SERVER_IP "202.120.38.131"

std::unordered_map<std::string, uint64_t> statistic_map;

int main(int argc, char const *argv[])
{
	int data_num = 1000;
	if (argc == 2)
		data_num = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket initialized error!\n");
        exit(-1);
    }

	struct sockaddr_in server_addr;
	char data[1024];
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

	printf("connected to server\n");

	int len, count = 0;

	while(1)
	{
		memset(data, 0, 1024);
		len = recv(sockfd, data, 1024, 0);
		//data[len] = '\0';
		printf("recv: %d\n", len);
		statistic_map[std::string(data)] = getMicros();
		count++;
		if (count == data_num)
			break;
	}
	
	close(sockfd);

	std::ofstream file;
	file.open("TCPTest1.txt");
	for (auto i = statistic_map.begin(); i != statistic_map.end(); ++i) {
		std::string past = (i->first).substr((i->first).find_last_of(":") + 1);
		//std::cout << i->first << "   " << past << std::endl;
		uint64_t time = (i->second) - std::stoull(past);
		file << i->first.substr(0, i->first.find_last_of("t"));
		file << " sendTime:" << past << " recvTime:" << i->second;
		file << "timeCost:" << time << '\n'; 
	}
	file.close();

    return 0;
}
