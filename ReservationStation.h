#ifndef RESERVATION_STATION_H
#define RESERVATION_STATION_H
#include <string>

// ReservationStation: representa uma estação de reserva no Tomasulo.
// Campos principais:
// - name/type: identificação e categoria (ADD/SUB, MUL/DIV, LOAD, STORE)
// - busy: se a RS está ocupada
// - op: operação em execução
// - vj/vk: valores de operandos quando já conhecidos
// - qj/qk: tags (ROB) dos produtores quando operandos não estão prontos
// - A: campo auxiliar (offset imediato ou parte do cálculo de endereço)
// - execCyclesRemaining/executing: controle local da execução
// - instrIndex: índice da instrução correspondente na lista global
// - robId: id da entrada no ROB associada
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
        int robId = -1;

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
            robId = -1;
        }
};

#endif
