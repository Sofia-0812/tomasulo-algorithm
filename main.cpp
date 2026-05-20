#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "ReservationStation.h"
#include "Instruction.h"

std::ofstream outFile;

int getLatency(std::string op) {
    if (op == "ADD" || op == "SUB") return 2;
    if (op == "MUL") return 10;
    if (op == "DIV") return 40;
    if (op == "LW" || op == "SW") return 2;
    return 1;
}

std::string getRsType(std::string op) {
    if (op == "ADD" || op == "SUB") return "ADD/SUB";
    if (op == "MUL" || op == "DIV") return "MUL/DIV";
    if (op == "LW") return "LOAD";
    if (op == "SW") return "STORE";
    return "";
}

bool hasWork(std::vector<Instruction>& instructions, int nextIssue, std::vector<ReservationStation>& rs) {
    if (nextIssue < (int)instructions.size()) return true;
    for (int i = 0; i < (int)rs.size(); i++) {
        if (rs[i].busy) return true;
    }
    return false;
}

void printState(int cycle, std::vector<ReservationStation>& rs,
                std::unordered_map<std::string, std::string>& regStatus,
                std::unordered_map<std::string, float>& registers,
                std::vector<Instruction>& instructions) {
    outFile << "\nCiclo " << cycle << "\n";

    outFile << "\nReservation Stations:\n";
    outFile << "Nome;Busy;Op;Vj;Vk;Qj;Qk;A\n";
    for (int i = 0; i < (int)rs.size(); i++) {
        std::string busy;
        if (rs[i].busy) {
            busy = "Sim";
        } else {
            busy = "Nao";
        }
        outFile << rs[i].name << ";" << busy << ";" << rs[i].op << ";";
        if (rs[i].qj.empty() && rs[i].busy) {
            outFile << rs[i].vj;
        }
        outFile << ";";
        if (rs[i].qk.empty() && rs[i].busy) {
            outFile << rs[i].vk;
        }
        outFile << ";" << rs[i].qj << ";" << rs[i].qk << ";";
        if (rs[i].busy) {
            outFile << rs[i].A;
        }
        outFile << "\n";
    }

    outFile << "\nRegister Status:\n";
    for (std::pair<const std::string, std::string>& pair : regStatus) {
        if (!pair.second.empty()) {
            outFile << pair.first << ";" << pair.second << "\n";
        }
    }

    outFile << "\nRegister File:\n";
    for (std::pair<const std::string, float>& pair : registers) {
        outFile << pair.first << ";" << pair.second << "\n";
    }

    outFile << "\nInstrucoes:\n";
    outFile << "Instr;Issue;ExecIni;ExecFim;Write\n";
    for (int i = 0; i < (int)instructions.size(); i++) {
        std::string instrStr;
        if (instructions[i].op == "LW" || instructions[i].op == "SW") {
            instrStr = instructions[i].op + " " + instructions[i].r1 + " " + std::to_string(instructions[i].imm) + "(" + instructions[i].r2 + ")";
        } else {
            instrStr = instructions[i].op + " " + instructions[i].r1 + " " + instructions[i].r2 + " " + instructions[i].r3;
        }
        outFile << instrStr << ";";
        if (instructions[i].issueCycle != -1) outFile << instructions[i].issueCycle;
        outFile << ";";
        if (instructions[i].execStartCycle != -1) outFile << instructions[i].execStartCycle;
        outFile << ";";
        if (instructions[i].execEndCycle != -1) outFile << instructions[i].execEndCycle;
        outFile << ";";
        if (instructions[i].writeResultCycle != -1) outFile << instructions[i].writeResultCycle;
        outFile << "\n";
    }
}

void writeResult(int cycle, std::vector<ReservationStation>& rs,
                 std::unordered_map<std::string, std::string>& regStatus,
                 std::unordered_map<std::string, float>& registers,
                 std::vector<Instruction>& instructions) {
    for (int i = 0; i < (int)rs.size(); i++) {
        if (!rs[i].busy || !rs[i].executing || rs[i].execCyclesRemaining > 0) continue;

        float result = 0.0f;
        if (rs[i].op == "ADD") {
            result = rs[i].vj + rs[i].vk;
        } else if (rs[i].op == "SUB") {
            result = rs[i].vj - rs[i].vk;
        } else if (rs[i].op == "MUL") {
            result = rs[i].vj * rs[i].vk;
        } else if (rs[i].op == "DIV") {
            if (rs[i].vk != 0) {
                result = rs[i].vj / rs[i].vk;
            }
        } else if (rs[i].op == "LW") {
            result = (float)rs[i].A;
        }

        for (int j = 0; j < (int)rs.size(); j++) {
            if (!rs[j].busy) continue;
            if (rs[j].qj == rs[i].name) {
                rs[j].vj = result;
                rs[j].qj = "";
            }
            if (rs[j].qk == rs[i].name) {
                rs[j].vk = result;
                rs[j].qk = "";
            }
        }

        if (rs[i].instrIndex >= 0) {
            std::string dest = instructions[rs[i].instrIndex].r1;
            if (regStatus[dest] == rs[i].name) {
                registers[dest] = result;
                regStatus[dest] = "";
            }
            instructions[rs[i].instrIndex].writeResultCycle = cycle;
        }

        rs[i].clear();
        return;
    }
}

void executeInstruction(int cycle, std::vector<ReservationStation>& rs,
                        std::vector<Instruction>& instructions) {
    for (int i = 0; i < (int)rs.size(); i++) {
        if (!rs[i].busy) continue;

        if (rs[i].executing) {
            rs[i].execCyclesRemaining--;
            continue;
        }

        if (rs[i].qj.empty() && rs[i].qk.empty()) {
            rs[i].executing = true;
            rs[i].execCyclesRemaining = getLatency(rs[i].op) - 1;
            if (rs[i].instrIndex >= 0) {
                instructions[rs[i].instrIndex].execStartCycle = cycle;
                instructions[rs[i].instrIndex].execEndCycle = cycle + getLatency(rs[i].op) - 1;
            }
        }
    }
}

void issueInstruction(int cycle, std::vector<Instruction>& instructions, int& nextIssue,
                      std::vector<ReservationStation>& rs,
                      std::unordered_map<std::string, std::string>& regStatus,
                      std::unordered_map<std::string, float>& registers) {
    int issued = 0;
    while (issued < 2 && nextIssue < (int)instructions.size()) {
        Instruction& instr = instructions[nextIssue];
        std::string rsType = getRsType(instr.op);

        ReservationStation* freeRS = nullptr;
        for (int i = 0; i < (int)rs.size(); i++) {
            if (rs[i].type == rsType && !rs[i].busy) {
                freeRS = &rs[i];
                break;
            }
        }

        if (!freeRS) break;

        freeRS->busy = true;
        freeRS->op = instr.op;
        freeRS->instrIndex = nextIssue;

        if (instr.op == "LW") {
            freeRS->A = instr.imm;
            if (!regStatus[instr.r2].empty()) {
                freeRS->qj = regStatus[instr.r2];
            } else {
                freeRS->vj = registers[instr.r2];
            }
            freeRS->qk = "";
            freeRS->vk = 0;
        } else if (instr.op == "SW") {
            freeRS->A = instr.imm;
            if (!regStatus[instr.r1].empty()) {
                freeRS->qj = regStatus[instr.r1];
            } else {
                freeRS->vj = registers[instr.r1];
            }
            if (!regStatus[instr.r2].empty()) {
                freeRS->qk = regStatus[instr.r2];
            } else {
                freeRS->vk = registers[instr.r2];
            }
        } else {
            if (!regStatus[instr.r2].empty()) {
                freeRS->qj = regStatus[instr.r2];
            } else {
                freeRS->vj = registers[instr.r2];
            }
            if (!regStatus[instr.r3].empty()) {
                freeRS->qk = regStatus[instr.r3];
            } else {
                freeRS->vk = registers[instr.r3];
            }
        }

        if (instr.op != "SW") {
            regStatus[instr.r1] = freeRS->name;
        }

        instr.issueCycle = cycle;
        nextIssue++;
        issued++;
    }
}

std::vector<Instruction> parseFile(std::string filename) {
    std::vector<Instruction> instructions;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string op, r1, r2, r3;
        int imm = 0;

        iss >> op >> r1;

        if (op == "LW" || op == "SW") {
            std::string offset, base;
            iss >> offset >> base;
            imm = std::stoi(offset);
            r2 = base;
            r3 = "";
        } else {
            iss >> r2 >> r3;
        }

        instructions.push_back(Instruction(op, r1, r2, r3, imm));
    }
    return instructions;
}

int main(int argc, char* argv[]) {
    std::string filename = "instructions.txt";
    if (argc > 1) filename = argv[1];

    std::vector<Instruction> instructions = parseFile(filename);
    if (instructions.empty()) {
        std::cout << "Arquivo vazio ou nao encontrado: " << filename << "\n";
        return 1;
    }

    outFile.open("result.txt");

    std::vector<ReservationStation> rs;
    rs.push_back(ReservationStation("Add1", "ADD/SUB"));
    rs.push_back(ReservationStation("Add2", "ADD/SUB"));
    rs.push_back(ReservationStation("Add3", "ADD/SUB"));
    rs.push_back(ReservationStation("Mul1", "MUL/DIV"));
    rs.push_back(ReservationStation("Mul2", "MUL/DIV"));
    rs.push_back(ReservationStation("Load1", "LOAD"));
    rs.push_back(ReservationStation("Load2", "LOAD"));
    rs.push_back(ReservationStation("Str1", "STORE"));
    rs.push_back(ReservationStation("Str2", "STORE"));

    std::unordered_map<std::string, float> registers;
    registers["F0"] = 0.0f;
    registers["F2"] = 2.5f;
    registers["F4"] = 7.0f;
    registers["F6"] = 0.0f;
    registers["F8"] = 1.5f;
    registers["F10"] = 0.0f;
    registers["F12"] = 0.0f;
    registers["R2"] = 100.0f;
    registers["R3"] = 200.0f;

    std::unordered_map<std::string, std::string> regStatus;

    int cycle = 1;
    int nextIssue = 0;

    while (hasWork(instructions, nextIssue, rs)) {
        writeResult(cycle, rs, regStatus, registers, instructions);
        executeInstruction(cycle, rs, instructions);
        issueInstruction(cycle, instructions, nextIssue, rs, regStatus, registers);
        printState(cycle, rs, regStatus, registers, instructions);
        cycle++;
    }

    outFile << "\nSimulacao finalizada\n";
    outFile << "Total de ciclos: " << cycle - 1 << "\n";
    outFile.close();
    std::cout << "Resultado salvo em result.txt\n";

    return 0;
}
