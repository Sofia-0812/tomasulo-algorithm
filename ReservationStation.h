#ifndef RESERVATION_STATION_H
#define RESERVATION_STATION_H
#include <string>

class ReservationStation {
    public:
        std::string name;
        std::string type;
        bool busy = false;
        std::string op;
        float vj = 0.0f;
        float vk = 0.0f;
        std::string qj;
        std::string qk;
        int A = 0;
        int execCyclesRemaining = 0;
        bool executing = false;
        int instrIndex = -1;

        ReservationStation(std::string name, std::string type)
            : name(name), type(type) {}

        void clear() {
            busy = false;
            op = "";
            vj = vk = 0.0f;
            qj = qk = "";
            A = 0;
            execCyclesRemaining = 0;
            executing = false;
            instrIndex = -1;
        }
};

#endif
