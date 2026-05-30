#ifndef INSTRUCTION_H
#define INSTRUCTION_H
#include <string>

// Instruction: representa uma instrução do programa de entrada.
// Armazena os operandos/offsets e os ciclos onde cada estágio foi registrado
// (issue, exec start, exec end, write result, commit) para fins de rastreamento.
class Instruction {
    public:
        std::string op;
        std::string r1;
        std::string r2;
        std::string r3;
        int imm;
        int issueCycle = -1;
        int execStartCycle = -1;
        int execEndCycle = -1;
        int writeResultCycle = -1;
        int commitCycle = -1;

        Instruction(std::string op, std::string r1, std::string r2, std::string r3, int imm)
            : op(op), r1(r1), r2(r2), r3(r3), imm(imm) {}
};

#endif
