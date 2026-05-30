#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip> 

// Removidos os includes fictícios para manter o código auto-contido caso necessário,
// mas pode mantê-los se os seus arquivos .h estiverem na mesma pasta.
#ifndef RESERVATION_STATION_H
struct ReservationStation {
    std::string name;
    std::string type;
    bool busy = false;
    std::string op = "-";
    float vj = 0.0f;
    float vk = 0.0f;
    std::string qj = ""; 
    std::string qk = ""; 
    int A = 0;
    bool executing = false;
    int execCyclesRemaining = 0;
    int instrIndex = -1;
    int robId = -1;       

    ReservationStation(std::string n, std::string t) : name(n), type(t) {}
    void clear() {
        busy = false; op = "-"; vj = 0.0f; vk = 0.0f;
        qj = ""; qk = ""; A = 0; executing = false;
        execCyclesRemaining = 0; instrIndex = -1; robId = -1;
    }
};
#endif

#ifndef INSTRUCTION_H
struct Instruction {
    std::string op, r1, r2, r3;
    int imm;
    int issueCycle = -1, execStartCycle = -1, execEndCycle = -1, writeResultCycle = -1, commitCycle = -1;
    Instruction(std::string o, std::string reg1, std::string reg2, std::string reg3, int im)
        : op(o), r1(reg1), r2(reg2), r3(reg3), imm(im) {}
};
#endif

#ifndef ROB_ENTRY_H
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

std::ofstream outFile;

int getLatency(std::string op) {
    if (op == "ADD" || op == "SUB") return 2;
    if (op == "MUL") return 5;
    if (op == "DIV") return 5;
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

bool hasWork(std::vector<Instruction>& instructions, int nextIssue, 
             std::vector<ReservationStation>& rs, std::vector<ROBEntry>& rob) {
    if (nextIssue < (int)instructions.size()) return true;
    for (const auto& station : rs) if (station.busy) return true;
    for (const auto& entry : rob) if (entry.busy) return true;
    return false;
}

std::string formatFloat(float value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    std::string str = oss.str();
    if (str.find('.') != std::string::npos) {
        while (str.back() == '0') str.pop_back();
        if (str.back() == '.') str.pop_back();
    }
    return str;
}

void printState(int cycle, std::vector<ReservationStation>& rs,
                std::unordered_map<std::string, std::string>& regStatus,
                std::unordered_map<std::string, float>& registers,
                std::vector<Instruction>& instructions,
                std::vector<ROBEntry>& rob) {
    
    outFile << "=========================================================================================\n";
    outFile << "                                       CICLO " << cycle << "\n";
    outFile << "=========================================================================================\n";

    // --- 1. TABELA: REORDER BUFFER ---
    outFile << "\n[ REORDER BUFFER ]\n";
    outFile << "-----------------------------------------------------------------------------------------\n";
    outFile << std::left << std::setw(8)  << "Entrada" 
                         << std::setw(6)  << "Busy" 
                         << std::setw(30) << "Instrucao" 
                         << std::setw(15) << "Estado" 
                         << std::setw(15) << "Destino/Valor" << "\n";
    outFile << "-----------------------------------------------------------------------------------------\n";
    for (const auto& entry : rob) {
        if (!entry.busy) continue;
        
        // Garante que o estado exibido case perfeitamente com o ciclo de gravação
        std::string stateStr = "Execute";
        if (entry.ready) stateStr = "Write Result";
        
        std::string instStr = "-";
        if (entry.instrIndex >= 0) {
            const auto& inst = instructions[entry.instrIndex];
            if (inst.op == "LW" || inst.op == "SW") {
                instStr = inst.op + " " + inst.r1 + ", " + std::to_string(inst.imm) + "(" + inst.r2 + ")";
            } else {
                instStr = inst.op + " " + inst.r1 + ", " + inst.r2 + ", " + inst.r3;
            }
        }
        
        std::string destVal = entry.dest + " = " + (entry.ready ? formatFloat(entry.value) : "-");
        outFile << std::left << std::setw(8)  << ("#" + std::to_string(entry.id))
                             << std::setw(6)  << "Sim"
                             << std::setw(30) << instStr
                             << std::setw(15) << stateStr
                             << std::setw(15) << destVal << "\n";
    }
    outFile << "-----------------------------------------------------------------------------------------\n";

    // --- 2. TABELA: RESERVATION STATIONS ---
    outFile << "\n[ RESERVATION STATIONS ]\n";
    outFile << "-----------------------------------------------------------------------------------------\n";
    outFile << std::left << std::setw(8)  << "Nome" 
                         << std::setw(6)  << "Busy" 
                         << std::setw(6)  << "Op" 
                         << std::setw(10) << "Vj" 
                         << std::setw(10) << "Vk" 
                         << std::setw(8)  << "Qj" 
                         << std::setw(8)  << "Qk" 
                         << std::setw(6)  << "Dest(ROB)" << "\n";
    outFile << "-----------------------------------------------------------------------------------------\n";

    for (const auto& station : rs) {
        std::string busy = station.busy ? "Sim" : "Nao";
        std::string vjStr = (station.busy && station.qj.empty()) ? formatFloat(station.vj) : "-";
        std::string vkStr = (station.busy && station.qk.empty()) ? formatFloat(station.vk) : "-";
        std::string qjStr = (station.busy && !station.qj.empty()) ? station.qj : "-";
        std::string qkStr = (station.busy && !station.qk.empty()) ? station.qk : "-";
        std::string robDest = station.busy ? ("#" + std::to_string(station.robId)) : "-";

        outFile << std::left << std::setw(8)  << station.name 
                             << std::setw(6)  << busy 
                             << std::setw(6)  << (station.busy ? station.op : "-") 
                             << std::setw(10) << vjStr 
                             << std::setw(10) << vkStr 
                             << std::setw(8)  << qjStr 
                             << std::setw(8)  << qkStr 
                             << std::setw(6)  << robDest << "\n";
    }
    outFile << "-----------------------------------------------------------------------------------------\n";

    // --- 3. TABELA: REGISTER FILE & STATUS ---
    outFile << "\n[ REGISTER FILE & STATUS ]\n";
    outFile << "------------------------------------\n";
    outFile << std::left << std::setw(12) << "Registrador" 
                         << std::setw(12) << "Status (Qi)" 
                         << std::setw(10) << "Valor" << "\n";
    outFile << "------------------------------------\n";

    for (const auto& pair : registers) {
        std::string reg = pair.first;
        std::string status = regStatus[reg].empty() ? "Pronto" : regStatus[reg];
        outFile << std::left << std::setw(12) << reg 
                             << std::setw(12) << status 
                             << std::setw(10) << formatFloat(pair.second) << "\n";
    }
    outFile << "------------------------------------\n";

    // --- 4. TABELA: FLUXO DE INSTRUÇÕES ---
    outFile << "\n[ STATUS DAS INSTRUCOES ]\n";
    outFile << "---------------------------------------------------------------------------------------------------\n";
    outFile << std::left << std::setw(25) << "Instrucao" 
                         << std::setw(10) << "Issue" 
                         << std::setw(12) << "Exec Ini" 
                         << std::setw(12) << "Exec Fim" 
                         << std::setw(12) << "Write" 
                         << std::setw(10) << "Commit" << "\n";
    outFile << "---------------------------------------------------------------------------------------------------\n";

    for (const auto& inst : instructions) {
        std::string instrStr;
        if (inst.op == "LW" || inst.op == "SW") {
            instrStr = inst.op + " " + inst.r1 + ", " + std::to_string(inst.imm) + "(" + inst.r2 + ")";
        } else {
            instrStr = inst.op + " " + inst.r1 + ", " + inst.r2 + ", " + inst.r3;
        }

        auto formatCycle = [](int c) { return (c == -1) ? "-" : std::to_string(c); };

        outFile << std::left << std::setw(25) << instrStr
                             << std::setw(10) << formatCycle(inst.issueCycle)
                             << std::setw(12) << formatCycle(inst.execStartCycle)
                             << std::setw(12) << formatCycle(inst.execEndCycle)
                             << std::setw(12) << formatCycle(inst.writeResultCycle)
                             << std::setw(10) << formatCycle(inst.commitCycle) << "\n";
    }
    outFile << "---------------------------------------------------------------------------------------------------\n\n";
}

// ETAPA 4: COMMIT
void commitInstruction(int cycle, std::vector<ROBEntry>& rob, 
                       std::unordered_map<std::string, std::string>& regStatus,
                       std::unordered_map<std::string, float>& registers,
                       std::vector<Instruction>& instructions) {
    if (!rob.empty() && rob[0].busy && rob[0].ready) {
        ROBEntry committed = rob[0];
        
        if (committed.op != "SW") {
            std::string robTag = "#" + std::to_string(committed.id);
            if (regStatus[committed.dest] == robTag) {
                registers[committed.dest] = committed.value;
                regStatus[committed.dest] = "";
            }
        }

        if (committed.instrIndex >= 0) {
            instructions[committed.instrIndex].commitCycle = cycle;
        }

        rob.erase(rob.begin());
        
        ROBEntry newEntry;
        newEntry.id = rob.empty() ? 1 : rob.back().id + 1;
        rob.push_back(newEntry);
    }
}

// ETAPA 3: WRITE RESULT
void writeResult(int cycle, std::vector<ReservationStation>& rs,
                 std::vector<Instruction>& instructions, std::vector<ROBEntry>& rob) {
    for (int i = 0; i < (int)rs.size(); i++) {
        if (!rs[i].busy || !rs[i].executing || rs[i].execCyclesRemaining > 0) continue;

        float result = 0.0f;
        if (rs[i].op == "ADD")      result = rs[i].vj + rs[i].vk;
        else if (rs[i].op == "SUB") result = rs[i].vj - rs[i].vk;
        else if (rs[i].op == "MUL") result = rs[i].vj * rs[i].vk;
        else if (rs[i].op == "DIV") { if (rs[i].vk != 0) result = rs[i].vj / rs[i].vk; }
        // Ajuste conceitual: LW calcula o endereço efetivo (Imediato + RegBase)
        else if (rs[i].op == "LW")  result = (float)(rs[i].A + rs[i].vj); 

        std::string myRobTag = "#" + std::to_string(rs[i].robId);

        for (int j = 0; j < (int)rs.size(); j++) {
            if (!rs[j].busy) continue;
            if (rs[j].qj == myRobTag) { rs[j].vj = result; rs[j].qj = ""; }
            if (rs[j].qk == myRobTag) { rs[j].vk = result; rs[j].qk = ""; }
        }

        for (auto& entry : rob) {
            if (entry.busy && entry.id == rs[i].robId) {
                entry.value = result;
                entry.ready = true;
                break;
            }
        }

        if (rs[i].instrIndex >= 0) {
            instructions[rs[i].instrIndex].writeResultCycle = cycle;
        }

        rs[i].clear();
        return; 
    }
}

// ETAPA 2: EXECUTE INSTRUCTION
void executeInstruction(int cycle, std::vector<ReservationStation>& rs,
                        std::vector<Instruction>& instructions) {
    for (int i = 0; i < (int)rs.size(); i++) {
        if (!rs[i].busy) continue;

        if (rs[i].executing) {
            rs[i].execCyclesRemaining--;
            continue;
        }

        // Ajuste sutil de Sincronismo: Só inicia a execução se a instrução NÃO deu Issue
        // no exato ciclo atual (evitando pular etapas) e não tem dependências pendentes.
        if (rs[i].qj.empty() && rs[i].qk.empty() && instructions[rs[i].instrIndex].issueCycle < cycle) {
            rs[i].executing = true;
            rs[i].execCyclesRemaining = getLatency(rs[i].op) - 1;
            if (rs[i].instrIndex >= 0) {
                instructions[rs[i].instrIndex].execStartCycle = cycle;
                instructions[rs[i].instrIndex].execEndCycle = cycle + getLatency(rs[i].op) - 1;
            }
        }
    }
}

// ETAPA 1: ISSUE INSTRUCTION
void issueInstruction(int cycle, std::vector<Instruction>& instructions, int& nextIssue,
                      std::vector<ReservationStation>& rs,
                      std::unordered_map<std::string, std::string>& regStatus,
                      std::unordered_map<std::string, float>& registers,
                      std::vector<ROBEntry>& rob) {
    int issued = 0;
    while (issued < 2 && nextIssue < (int)instructions.size()) {
        Instruction& instr = instructions[nextIssue];
        std::string rsType = getRsType(instr.op);

        ROBEntry* freeROB = nullptr;
        for (auto& entry : rob) {
            if (!entry.busy) { freeROB = &entry; break; }
        }

        ReservationStation* freeRS = nullptr;
        for (int i = 0; i < (int)rs.size(); i++) {
            if (rs[i].type == rsType && !rs[i].busy) { freeRS = &rs[i]; break; }
        }

        if (!freeROB || !freeRS) break;

        freeROB->busy = true;
        freeROB->op = instr.op;
        freeROB->dest = instr.r1;
        freeROB->ready = false;
        freeROB->instrIndex = nextIssue;
        std::string robTag = "#" + std::to_string(freeROB->id);

        freeRS->busy = true;
        freeRS->op = instr.op;
        freeRS->instrIndex = nextIssue;
        freeRS->robId = freeROB->id;

        auto lerOperando = [&](std::string reg, std::string& qField, float& vField) {
            if (!regStatus[reg].empty()) {
                std::string provedor = regStatus[reg]; 
                int idProvedor = std::stoi(provedor.substr(1));
                
                bool achouPronto = false;
                for (const auto& e : rob) {
                    if (e.busy && e.id == idProvedor && e.ready) {
                        vField = e.value;
                        qField = "";
                        achouPronto = true;
                        break;
                    }
                }
                if (!achouPronto) qField = provedor;
            } else {
                vField = registers[reg];
                qField = "";
            }
        };

        if (instr.op == "LW") {
            freeRS->A = instr.imm;
            lerOperando(instr.r2, freeRS->qj, freeRS->vj);
            freeRS->qk = ""; freeRS->vk = 0;
        } else if (instr.op == "SW") {
            freeRS->A = instr.imm;
            lerOperando(instr.r1, freeRS->qj, freeRS->vj);
            lerOperando(instr.r2, freeRS->qk, freeRS->vk);
        } else {
            lerOperando(instr.r2, freeRS->qj, freeRS->vj);
            lerOperando(instr.r3, freeRS->qk, freeRS->vk);
        }

        if (instr.op != "SW") {
            regStatus[instr.r1] = robTag;
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
            r2 = base; r3 = "";
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

    std::vector<ROBEntry> rob(6);
    for (int i = 0; i < 6; i++) rob[i].id = i + 1;

    std::unordered_map<std::string, float> registers;
    registers["F0"] = 0.0f;   registers["F2"] = 2.5f;   registers["F4"] = 7.0f;
    registers["F6"] = 0.0f;   registers["F8"] = 1.5f;   registers["F10"] = 0.0f;
    registers["F12"] = 0.0f;  registers["R2"] = 100.0f; registers["R3"] = 200.0f;

    std::unordered_map<std::string, std::string> regStatus;

    int cycle = 1;
    int nextIssue = 0;

    while (hasWork(instructions, nextIssue, rs, rob)) {
        commitInstruction(cycle, rob, regStatus, registers, instructions);
        writeResult(cycle, rs, instructions, rob);
        executeInstruction(cycle, rs, instructions);
        issueInstruction(cycle, instructions, nextIssue, rs, regStatus, registers, rob);
        printState(cycle, rs, regStatus, registers, instructions, rob);
        cycle++;
    }

    outFile << "\nSimulacao finalizada com ROB\n";
    outFile << "Total de ciclos: " << cycle - 1 << "\n";
    outFile.close();
    std::cout << "Resultado com ROB salvo com sucesso em result.txt\n";

    return 0;
}