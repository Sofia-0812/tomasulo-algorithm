#ifndef ROB_ENTRY_H
#define ROB_ENTRY_H
#include <string>

struct ROBEntry {
    int id;
    bool busy = false;
    std::string op;
    std::string dest;
    float value = 0.0f;
    bool ready = false;
    int instrIndex = -1;
};

#endif