#ifndef ROB_ENTRY_H
#define ROB_ENTRY_H
#include <string>

// ROBState: estado explícito de uma entrada do Reorder Buffer.
enum class ROBState {
    Free,
    Issued,
    Executing,
    WriteResult,
    Commit
};

// ROBEntry: entrada do Reorder Buffer (ROB).
// O ROB implementa commit in-order, enquanto permite execução out-of-order.
// Campos:
// - id: identificador único da entrada, é com essa tag que a renomeação de registradores funciona
// - busy: se a entrada está sendo usada
// - op: operação ("ADD","LW","SW",...)
// - dest: registrador destino (vazio para stores)
// - value: valor produzido pela instrução
// - ready: indica que o valor já foi calculado e pode ser commitado
// - instrIndex: índice da instrução original na lista de instruções
// - addr: endereço efetivo para operações de memória (LW/SW)
struct ROBEntry {
    int id = -1;
    bool busy = false;
    std::string op;
    std::string dest;
    float value = 0.0f;
    bool ready = false;
    int instrIndex = -1;
    int addr = 0; // endereço de memória efetivo para loads/stores
    bool addrValid = false; // true quando 'addr' foi computado 
    ROBState state = ROBState::Free;

    // clear: volta a entrada para o estado livre antes de reutilizar o slot.
    // Mantemos essa rotina no próprio tipo para centralizar a semântica.
    void clear() {
        id = -1;
        busy = false;
        op.clear();
        dest.clear();
        value = 0.0f;
        ready = false;
        instrIndex = -1;
        addr = 0;
        addrValid = false;
        state = ROBState::Free;
    }
};

#endif