#include <fstream>

int main(int argc, char const *argv[])
{
    std::string fileName = std::string(argv[1]);
    std::ifstream in;
    in.open(fileName);
    std::string content;
    std::ofstream file;
	file.open("cost_"+fileName);
    while (std::getline(in, content))
    {
        std::string part1 = content.substr(content.find_last_of(":")+1);
        file << part1 << '\n';
    }
    in.close();
    file.close();
    return 0;
}
