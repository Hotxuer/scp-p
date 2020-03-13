#include <fstream>

int main(int argc, char const *argv[])
{
    uint64_t num = atoi(argv[1]);
    std::string fileName = std::string(argv[2]);
    std::ifstream in;
    in.open(fileName);
    std::string content;
    std::ofstream file;
	file.open("_"+fileName);
    while (std::getline(in, content))
    {
        std::string part1 = content.substr(0, content.find("recvTime:"));
        part1 += "recvTime:";
        uint64_t recvTime = std::stoull(content.substr(content.find("recvTime:")+9, content.find("recvTime")+25));
        recvTime += num;
        part1 += std::to_string(recvTime);
        part1 += " timeCost:";
        uint64_t cost = std::stoull(content.substr(content.find_last_of(":")+1));
        cost += num;
        part1 += std::to_string(cost);
        file << part1 << '\n';
    }
    return 0;
}
