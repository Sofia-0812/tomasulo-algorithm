#ifndef ROB_ENTRY_H
#define ROB_ENTRY_H
#include <string>

// ROBEntry: entrada do Reorder Buffer (ROB).
// O ROB implementa commit in-order, enquanto permite execução out-of-order.
// Campos:
// - id: identificador único da entrada
// - busy: se a entrada está sendo usada
// - op: operação ("ADD","LW","SW",...)
// - dest: registrador destino (vazio para stores)
// - value: valor produzido pela instrução
// - ready: indica que o valor já foi calculado e pode ser commitado
// - instrIndex: índice da instrução original na lista de instruções
// - addr: endereço efetivo para operações de memória (LW/SW)
struct ROBEntry {
    int id;
    bool busy = false;
    std::string op;
    std::string dest;
    float value = 0.0f;
    bool ready = false;
    int instrIndex = -1;
    int addr = 0; // effective memory address for loads/stores
};

#endif