all: server1 server2 server3 client1 client2 client3

server1: TCPServer1.cc
	g++ -o server1 TCPServer1.cc -lpthread

server2: TCPServer2.cc
	g++ -o server2 TCPServer2.cc -lpthread

server3: TCPServer3.cc
	g++ -o server3 TCPServer3.cc -lpthread

client1: TCPClient1.cc
	g++ -o client1 TCPClient1.cc

client2: TCPClient2.cc
	g++ -o client2 TCPClient2.cc

client1: TCPClient3.cc
	g++ -o client3 TCPClient3.cc

clean:
	rm server1 server2 server3 client1 client2 client3