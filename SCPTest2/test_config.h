#include <random>
struct SendConfig{
    int pkt_num;
    int min_period;
    int max_period;
    int big_packet;
};

void query_config(std::vector<SendConfig> configs){
    using std::cout;
    using std::endl;
    cout<<"mode_id  packet_num   min_period   max_period   is_big_packet"<<endl;
    int ind = 0;
    for(auto i : configs){
        cout<<ind++<<" | "<< i.pkt_num << "  |  " << i.min_period << "  |  " << i.max_period << "  |  " << i.big_packet <<endl;
    }
}

