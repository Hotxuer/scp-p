#include "include/common_lib.h"
uint64_t getSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count(); 
}


// Get time stamp in milliseconds.
uint64_t getMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count(); 
}

// Get time stamp in microseconds.
uint64_t getMicros()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    // return us; 
}

uint64_t getMillsDiff(uint64_t past) {
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    return now - past;
}

uint64_t getMicrosDiff(uint64_t past) {
    uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::
                  now().time_since_epoch()).count();
    return now - past;
}

void generate_rand_str(unsigned char* str, uint32_t len) {
    char base = 'A';
    int range = 58;
    srand((int)time(0));
    memset(str, 0, len);
    for (int i = 0; i < len - 1; i++)
    {
        *(str+i) = (unsigned char)(base + (rand() % range));
    }
}