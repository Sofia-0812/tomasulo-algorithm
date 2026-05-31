#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip> 

#include "ReservationStation.h"
#include "Instruction.h"
#include "ROBEntry.h"
#include "ReorderBuffer.h"
#include "StoreBuffer.h"

// main.cpp: implementação do simulador Tomasulo e do loop de clock.
// As estruturas básicas (`Instruction`, `ReservationStation`, `ROBEntry`) estão
// definidas em seus respectivos headers para facilitar leitura e documentação.

std::ofstream outFile;

// robStateToString: converte o estado interno do ROB em texto para o log.
std::string robStateToString(ROBState state) {
    switch (state) {
        case ROBState::Issued: return "Issue";
        case ROBState::Executing: return "Execute";
        case ROBState::WriteResult: return "Write result";
        case ROBState::Commit: return "Commit";
        case ROBState::Free: default: return "-";
    }
}

// getLatency: retorna a latência (em ciclos) associada a cada operação.
// Aqui determinamos quantos ciclos de clock cada operação vai gastar na etapa de Execução.
int getLatency(std::string op) {
    if (op == "ADD" || op == "SUB") return 2;
    if (op == "MUL") return 5;
    if (op == "DIV") return 5;
    if (op == "LW" || op == "SW") return 2;
    return 1;
}

// getRsType: mapeia uma operação ao tipo de Reservation Station necessária.
// Lê o mnemônico (ex: ADD) e diz para qual Estação de Reserva ele deve ser enviado.
// Retorna "ADD/SUB", "MUL/DIV", "LOAD" ou "STORE".
std::string getRsType(std::string op) {
    if (op == "ADD" || op == "SUB") return "ADD/SUB";
    if (op == "MUL" || op == "DIV") return "MUL/DIV";
    if (op == "LW") return "LOAD";
    if (op == "SW") return "STORE";
    return "";
}

// hasWork: determina se ainda há trabalho a ser feito (instruções a emitir,
// reservation stations ocupadas ou entradas ativas no ROB ou no StoreBuffer).
bool hasWork(std::vector<Instruction>& instructions, int nextIssue, 
             std::vector<ReservationStation>& rs, const CircularROB& rob,
             std::vector<StoreBufferEntry>& sb) {
    if (nextIssue < (int)instructions.size()) return true;
    for (const auto& station : rs) if (station.busy) return true;
    if (rob.size() > 0) return true;
    for (const auto& s : sb) if (s.busy) return true;
    return false;
}

// formatFloat: formata floats com até 2 casas significativas, removendo
// zeros não significativos. Usado apenas para melhorar a legibilidade do log.
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

// printState: escreve o estado completo do simulador em `result.txt`.
// Inclui ROB, Reservation Stations, Register File/Status e a tabela de instruções
// com os ciclos de issue/exec/write/commit. Essa função é puramente de
// apresentação/log e não altera o estado da simulação.
void printState(int cycle, std::vector<ReservationStation>& rs,
                std::unordered_map<std::string, std::string>& regStatus,
                std::unordered_map<std::string, float>& registers,
                std::vector<Instruction>& instructions,
                const CircularROB& rob,
                std::vector<StoreBufferEntry>& sb) {
    
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
    for (int i = 0; i < rob.capacity(); i++) {
        const auto& entry = rob.slotAt(i);
        
        // Garante que o estado exibido case perfeitamente com o ciclo de gravação
        std::string stateStr = robStateToString(entry.state);
        if (stateStr == "-" && entry.busy) stateStr = entry.ready ? "Write result" : "Issue";
        
        std::string instStr = "-";
        if (entry.busy && entry.instrIndex >= 0) {
            const auto& inst = instructions[entry.instrIndex];
            if (inst.op == "LW" || inst.op == "SW") {
                instStr = inst.op + " " + inst.r1 + ", " + std::to_string(inst.imm) + "(" + inst.r2 + ")";
            } else {
                instStr = inst.op + " " + inst.r1 + ", " + inst.r2 + ", " + inst.r3;
            }
        }
        
        std::string destVal;
        if (entry.busy && entry.op == "SW") {
            destVal = (entry.ready ? ("[" + std::to_string(entry.addr) + "]=" + formatFloat(entry.value)) : "-");
        } else if (entry.busy) {
            destVal = entry.dest + " = " + (entry.ready ? formatFloat(entry.value) : "-");
        } else {
            destVal = "-";
        }
        outFile << std::left << std::setw(8)  << (entry.id > 0 ? ("#" + std::to_string(entry.id)) : "-")
                             << std::setw(6)  << (entry.busy ? "Sim" : "Nao")
                             << std::setw(30) << instStr
                             << std::setw(15) << stateStr
                             << std::setw(15) << destVal << "\n";
    }
    outFile << "-----------------------------------------------------------------------------------------\n";

    // --- 1.5. TABELA: STORE BUFFER (opcional) ---
    outFile << "\n[ STORE BUFFER ]\n";
    outFile << "-----------------------------------------------------\n";
    outFile << std::left << std::setw(8) << "Slot" << std::setw(8) << "Busy" << std::setw(10) << "ROB#" << std::setw(10) << "Addr" << std::setw(10) << "Value" << "\n";
    outFile << "-----------------------------------------------------\n";
    for (int i = 0; i < (int)sb.size(); i++) {
        const auto& s = sb[i];
        outFile << std::left << std::setw(8) << ("S" + std::to_string(i)) << std::setw(8) << (s.busy?"Sim":"Nao") << std::setw(10) << (s.robId>0?("#"+std::to_string(s.robId)):"-") << std::setw(10) << (s.ready?std::to_string(s.addr):"-") << std::setw(10) << (s.ready?formatFloat(s.value):"-") << "\n";
    }
    outFile << "-----------------------------------------------------\n";

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
// commitInstruction: aplica o commit da entrada mais velha do ROB se ela estiver
// pronta. O ROB é circular e de tamanho fixo: o slot do head é commitado e o
// ponteiro avança, preservando a ordem in-order exigida pelo algoritmo.
void commitInstruction(int cycle, CircularROB& rob, 
                       std::unordered_map<std::string, std::string>& regStatus,
                       std::unordered_map<std::string, float>& registers,
                       std::vector<Instruction>& instructions,
                       std::unordered_map<int, float>& memory,
                       std::vector<StoreBufferEntry>& sb) {

    // 1. Pega a instrução mais antiga. Se ela não existir ou ainda estiver calculando, cancela.
    ROBEntry* front = rob.front();
    if (front == nullptr || !front->busy || !front->ready) return;

    // 2. Faz uma cópia de segurança e marca visualmente que entrou em commit.
    ROBEntry committed = *front;
    front->state = ROBState::Commit;

    if (committed.op == "SW") {
        // Se for gravação (Store), procura o valor correspondente no StoreBuffer
        // e efetua a gravação real e destrutiva em 'memory'.
        bool wrote = false;
        for (auto& s : sb) {
            if (s.busy && s.robId == committed.id && s.ready) {
                memory[s.addr] = s.value;
                s.busy = false; // Libera o slot do StoreBuffer
                wrote = true;
                break;
            }
        }
        // Fallback de segurança caso não esteja no StoreBuffer.
        if (!wrote) {
            memory[committed.addr] = committed.value;
        }
    } else {
        // Resolve Dependências de Saída (Write-After-Write):
        // Só atualizamos e liberamos o registrador se ele ainda estiver apontando 
        // para a Tag desta instrução. Se uma instrução mais nova já o renomeou, 
        // ignoramos para não sobrescrever a Tag nova e corromper o programa.
        std::string robTag = "#" + std::to_string(committed.id);
        if (!committed.dest.empty() && regStatus[committed.dest] == robTag) {
            registers[committed.dest] = committed.value;
            regStatus[committed.dest] = "";
        }
    }

    // 3. Anota o tempo no log.
    if (committed.instrIndex >= 0) {
        instructions[committed.instrIndex].commitCycle = cycle;
    }

    // 4. Libera o slot do ROB para ser usado por novas instruções.
    rob.commitFront();
}

// ETAPA 3: WRITE RESULT
// writeResult: verifica se alguma Reservation Station terminou a execução
// (execCyclesRemaining == 0) e então coloca o resultado no ROB correspondente
// e no CDB (simulado atualizando Qj/Qk de outras RS). Apenas 1 resultado é
// escrito por ciclo (a função retorna após processar o primeiro result-ready).
void writeResult(int cycle, std::vector<ReservationStation>& rs,
                 std::vector<Instruction>& instructions, CircularROB& rob,
                 std::unordered_map<int, float>& memory,
                 std::vector<StoreBufferEntry>& sb) {
    for (int i = 0; i < (int)rs.size(); i++) {
        // Ignora estações vazias ou que ainda não terminaram o tempo (countdown > 0)
        if (!rs[i].busy || !rs[i].executing || rs[i].execCyclesRemaining > 0) continue;

        // 1. Executa a matemática baseada no operador.
        float result = 0.0f;
        if (rs[i].op == "ADD")      result = rs[i].vj + rs[i].vk;
        else if (rs[i].op == "SUB") result = rs[i].vj - rs[i].vk;
        else if (rs[i].op == "MUL") result = rs[i].vj * rs[i].vk;
        else if (rs[i].op == "DIV") { if (rs[i].vk != 0) result = rs[i].vj / rs[i].vk; }
        else if (rs[i].op == "LW")  {
            int addr = rs[i].A + (int)rs[i].vj; // (endereco efetivo = A + base)
            // Lógica de desambiguação de memória (Store Forwarding):
            // Se houver algum store (escrita) mais antigo para o mesmo endereço no ROB,
            // devemos usar o valor desse store (se estiver pronto) ou bloquear (stall) o load
            // até que o store calcule o seu endereço/valor.

            // Variáveis de controle: 
            // stallForStore = true se encontrarmos um Store conflitante que ainda não está pronto.
            // forwarded = true se conseguirmos copiar o valor diretamente de um Store finalizado no ROB.
            bool stallForStore = false;
            bool forwarded = false;

            // Varre o ROB do mais antigo (head) para o mais novo, procurando Stores.
            rob.forEachActive([&](const ROBEntry& entry) {
                // 1. Ignora instruções mais novas que este Load (entry.id >= rs[i].robId).
                // 2. Para de procurar se já fizemos forwarding (achamos o valor) ou se decidimos dar Stall (conflito pendente).
                if (entry.id >= rs[i].robId || forwarded || stallForStore) return;
                if (entry.op == "SW") {
                    // Se o Store mais antigo ainda não calculou seu endereço, não sabemos se ele vai
                    // colidir com o nosso Load. Por segurança, bloqueamos (Stall) o Load.
                    if (!entry.addrValid) { stallForStore = true; return; }
                    // Se o Store já tem endereço e é o mesmo que o Load quer ler:
                    if (entry.addr == addr) {
                        // Se o dado do Store já foi calculado, copiamos o valor direto do ROB 
                        // (Store Forwarding), poupando tempo de acesso à memória.
                        if (entry.ready) {
                            result = entry.value;
                            forwarded = true;
                        } else {
                            // O endereço é o mesmo, mas o Store ainda está calculando o dado.
                            // O Load é obrigado a dar Stall e esperar.
                            stallForStore = true;
                        }
                    }
                }
            });
            if (stallForStore) continue; // Congela se houver conflito!
            if (!forwarded) {
                // Lê da memória física se não houve encaminhamento do ROB.
                if (memory.find(addr) != memory.end()) result = memory[addr];
                else result = (float)addr;
            }
        }
        else if (rs[i].op == "SW") {
            // Stores repassam os valores de endereço e dado calculados diretamente ao atualizar o ROB.
        }

        // 2. Transmissão no CDB (Common Data Bus)
        std::string myRobTag = "#" + std::to_string(rs[i].robId);

        for (int j = 0; j < (int)rs.size(); j++) {
            if (!rs[j].busy) continue;
            // Se alguém estava esperando a Tag desta instrução, entrega o valor e limpa a dependência.
            if (rs[j].qj == myRobTag) { rs[j].vj = result; rs[j].qj = ""; }
            if (rs[j].qk == myRobTag) { rs[j].vk = result; rs[j].qk = ""; }
        }

        // 3. Atualiza o slot correspondente no ROB informando que o cálculo terminou.
        ROBEntry* entry = rob.findById(rs[i].robId);
        if (entry != nullptr) {
            if (rs[i].op == "SW") {
                // Para Store, computa o endereço efetivo e então guarda endereço e valor no ROB 
                // e repassa ao Store Buffer.
                int addr = rs[i].A + (int)rs[i].vk;
                entry->addr = addr;
                entry->addrValid = true;
                entry->value = rs[i].vj;
                entry->ready = true;
                entry->state = ROBState::WriteResult;

                // Move os dados preparados para a sala de espera do StoreBuffer
                for (auto& s : sb) {
                    if (!s.busy) {
                        s.busy = true;
                        s.robId = entry->id;
                        s.addr = addr;
                        s.value = entry->value;
                        s.ready = true;
                        break;
                    }
                }
            } else if (rs[i].op == "LW") {
                // Grava o resultado da leitura da memória no ROB.
                int addr = rs[i].A + (int)rs[i].vj;
                entry->addr = addr;
                entry->addrValid = true;
                entry->value = result;
                entry->ready = true;
                entry->state = ROBState::WriteResult;
            } else {
                // Resultados matemáticos diretos.
                entry->value = result;
                entry->ready = true;
                entry->state = ROBState::WriteResult;
            }
        }

        if (rs[i].instrIndex >= 0) {
            instructions[rs[i].instrIndex].writeResultCycle = cycle;
        }

        // 4. Limpa a Estação de Reserva para ela receber nova instrução.
        rs[i].clear();
        return; // Apenas 1 gravação por ciclo
    }
}

// ETAPA 2: EXECUTE INSTRUCTION
// executeInstruction: inicia a execução de uma RS quando ambos operandos
// estiverem prontos e a instrução não foi emitida no ciclo atual; decrementa
// o contador de ciclos de execução para instruções já em execução.
void executeInstruction(int cycle, std::vector<ReservationStation>& rs,
                        std::vector<Instruction>& instructions,
                        CircularROB& rob) {
    for (int i = 0; i < (int)rs.size(); i++) {
        if (!rs[i].busy) continue;

        // Se a instrução já está rodando, apenas desconta 1 ciclo do tempo de processamento.
        if (rs[i].executing) {
            rs[i].execCyclesRemaining--;
            continue;
        }

        // Ajuste de Sincronismo: Só inicia a execução se a instrução NÃO deu Issue
        // no exato ciclo atual (evitando pular etapas) e não tem dependências pendentes.
        // Se os dois operandos estão prontos (qj e qk estão vazios) e a 
        // instrução não entrou neste mesmíssimo ciclo (issueCycle < cycle).
        if (rs[i].qj.empty() && rs[i].qk.empty() && instructions[rs[i].instrIndex].issueCycle < cycle) {
            rs[i].executing = true;
            // Pega o tempo da operação e começa o contador.
            rs[i].execCyclesRemaining = getLatency(rs[i].op) - 1;
            // Atualiza os registros de tempo para o log final
            if (rs[i].instrIndex >= 0) {
                instructions[rs[i].instrIndex].execStartCycle = cycle;
                instructions[rs[i].instrIndex].execEndCycle = cycle + getLatency(rs[i].op) - 1;
                ROBEntry* entry = rob.findById(rs[i].robId);
                if (entry != nullptr) entry->state = ROBState::Executing;
            }
        }
    }
}

// ETAPA 1: ISSUE INSTRUCTION
// issueInstruction: despacha até duas instruções por ciclo para RSs livres
// e reserva uma entrada no ROB. Também resolve fontes de operandos via
// Register Status e ROB forwarding quando possível.
void issueInstruction(int cycle, std::vector<Instruction>& instructions, int& nextIssue,
                      std::vector<ReservationStation>& rs,
                      std::unordered_map<std::string, std::string>& regStatus,
                      std::unordered_map<std::string, float>& registers,
                      CircularROB& rob) {
    int issued = 0;
    // Superescalaridade: tenta puxar até 2 instruções por ciclo.
    while (issued < 2 && nextIssue < (int)instructions.size()) {
        Instruction& instr = instructions[nextIssue];
        std::string rsType = getRsType(instr.op);

        // Verifica gargalos estruturais (precisa ter ROB e RS livres)
        ROBEntry* freeROB = rob.allocate();
        ReservationStation* freeRS = nullptr;
        for (int i = 0; i < (int)rs.size(); i++) {
            if (rs[i].type == rsType && !rs[i].busy) { freeRS = &rs[i]; break; }
        }

        // Se faltar recurso de hardware, encerra o loop (Stall).
        if (!freeROB || !freeRS) break;

        // Associa os dados base da instrução recém despachada no ROB
        freeROB->op = instr.op;
        freeROB->dest = (instr.op == "SW") ? "" : instr.r1;
        freeROB->ready = false;
        freeROB->instrIndex = nextIssue;
        freeROB->state = ROBState::Issued;

        // Gera a nova Tag que será propagada para quem depender deste resultado 
        std::string robTag = "#" + std::to_string(freeROB->id);

        // Configura a "sala de espera" da unidade funcional correspondente
        freeRS->busy = true;
        freeRS->op = instr.op;
        freeRS->instrIndex = nextIssue;
        freeRS->robId = freeROB->id;

        // lerOperando: Função auxiliar lambda de busca de operandos e verificação de RAW (Read-After-Write)
        auto lerOperando = [&](std::string reg, std::string& qField, float& vField) {
            if (!regStatus[reg].empty()) {
                // Se o status do registrador tiver uma Tag (ex: #2), ele está esperando cálculo
                std::string provedor = regStatus[reg]; 
                int idProvedor = std::stoi(provedor.substr(1));
                
                bool achouPronto = false;
                // Olha no ROB para verificar se o cálculo provedor já finalizou a execução
                for (int idx = 0; idx < rob.capacity(); idx++) {
                    const auto& e = rob.slotAt(idx);
                    if (e.busy && e.id == idProvedor && e.ready) {
                        vField = e.value; // Pega o valor real diretamente
                        qField = ""; // Limpa a pendência
                        achouPronto = true;
                        break;
                    }
                }
                // Se o provedor continua trabalhando, anota a dependência (Tag) no campo Qj/Qk da RS
                if (!achouPronto) qField = provedor;
            } else {
                // Se não há tag pendente, o valor no banco de registradores é o atual e oficial.
                vField = registers[reg];
                qField = "";
            }
        };

        // Direcionamento e extração de imediatos e registradores baseado no tipo da instrução
        if (instr.op == "LW") {
            freeRS->A = instr.imm; // Guarda offset da memória na variável Address (A)
            lerOperando(instr.r2, freeRS->qj, freeRS->vj); // Avalia a base
            freeRS->qk = ""; freeRS->vk = 0;
        } else if (instr.op == "SW") {
            freeRS->A = instr.imm;
            lerOperando(instr.r1, freeRS->qj, freeRS->vj); // Avalia o dado a ser gravado
            lerOperando(instr.r2, freeRS->qk, freeRS->vk); // Avalia a base
        } else {
            lerOperando(instr.r2, freeRS->qj, freeRS->vj); // Operando Matemático 1
            lerOperando(instr.r3, freeRS->qk, freeRS->vk); // Operando Matemático 2
        }

        // Renomeação de Registradores:
        // O registrador destino passa a apontar para a Tag (#ID) do ROB que acabou de ser criado.
        // Resolve dependências de nome e saída (WAW e WAR).
        if (instr.op != "SW") {
            regStatus[instr.r1] = robTag;
        }

        instr.issueCycle = cycle;
        nextIssue++;
        issued++;
    }
}

// parseFile: Lê o arquivo 'instructions.txt' e converte as strings em estruturas Instruction instanciáveis.
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

// parseMemoryFile: Inicializa o array simulado da memória principal a partir de 'memory.txt'
std::unordered_map<int, float> parseMemoryFile(const std::string& filename) {
    std::unordered_map<int, float> mem;
    std::ifstream file(filename);
    if (!file) return mem;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        std::istringstream iss(line);
        int addr; float val;
        if (!(iss >> addr >> val)) continue;
        mem[addr] = val;
    }
    return mem;
}

// Inicializa componentes do hardware e coordena o avanço síncrono do sinal de Clock global.
int main(int argc, char* argv[]) {

    // Leitura de arquivos 
    std::string filename = "instructions.txt";
    std::string memoryFilename = "memory.txt";
    if (argc > 1) filename = argv[1];
    if (argc > 2) memoryFilename = argv[2];

    std::vector<Instruction> instructions = parseFile(filename);
    if (instructions.empty()) {
        std::cout << "Arquivo vazio ou nao encontrado: " << filename << "\n";
        return 1;
    }

    outFile.open("result.txt");

    // Instanciação física das unidades executivas distribuídas em Estações
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

    // Inicialização da janela de restrição paralela (Capacidade 6)
    CircularROB rob(6);

    // Store buffer: pequena estrutura para segurar stores prontos antes do
    // commit; tamanho configurável conforme necessidade didática.
    std::vector<StoreBufferEntry> storeBuffer(4);

    // Prepara/inicializa o banco de registradores. 
    // Injeta valores iniciais para que as operações matemáticas tenham dados reais 
    // para calcular.
    std::unordered_map<std::string, float> registers;
    registers["F0"] = 0.0f;   registers["F2"] = 2.5f;   registers["F4"] = 7.0f;
    registers["F6"] = 0.0f;   registers["F8"] = 1.5f;   registers["F10"] = 0.0f;
    registers["F12"] = 0.0f;  registers["R2"] = 100.0f; registers["R3"] = 200.0f;

    // Cria a tabela de renomeação de registradores limpa (ninguém está esperando resultados pendentes).
    std::unordered_map<std::string, std::string> regStatus;

    // Cria a memoria principal simulada
    // Preenche endereços específicos (ex: 32 + R2 = 132) para que as instruções LW não leiam lixo.
    std::unordered_map<int, float> memory;
    memory[132] = 132.0f;
    memory[244] = 244.0f;
    memory[116] = 0.0f;

    // Permite que o usuário sobrescreva a memória através do arquivo 'memory.txt'
    auto fileMem = parseMemoryFile(memoryFilename);
    for (const auto& p : fileMem) memory[p.first] = p.second;

    int cycle = 1;
    int nextIssue = 0;

    // Roda enquanto o pipeline não estiver 100% vazio.
    // As funções são chamadas de trás para frente. Isso impede que um dado produzido no passo 1 
    // seja consumido magicamente no passo 2 durante o mesmo ciclo de clock (o que seria impossível na física do chip).
    while (hasWork(instructions, nextIssue, rs, rob, storeBuffer)) {
        commitInstruction(cycle, rob, regStatus, registers, instructions, memory, storeBuffer);
        writeResult(cycle, rs, instructions, rob, memory, storeBuffer);
        executeInstruction(cycle, rs, instructions, rob);
        issueInstruction(cycle, instructions, nextIssue, rs, regStatus, registers, rob);
        printState(cycle, rs, regStatus, registers, instructions, rob, storeBuffer);
        cycle++;
    }

    outFile << "\nSimulacao finalizada com ROB\n";
    outFile << "Total de ciclos: " << cycle - 1 << "\n";
    outFile.close();
    std::cout << "Resultado com ROB salvo com sucesso em result.txt\n";

    return 0;
}