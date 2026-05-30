#ifndef REORDER_BUFFER_H
#define REORDER_BUFFER_H

#include <vector>
#include "ROBEntry.h"

// CircularROB: implementação de um Reorder Buffer circular com capacidade fixa.
//
// Objetivo didático:
// - head aponta a entrada mais velha ainda ativa.
// - tail aponta o próximo slot livre para alocação.
// - count informa quantas entradas estão ocupadas.
// - nextId gera tags únicas para dependência (Qi/Qj/Qk) sem reutilização.
//
// A estrutura preserva a ordem in-order do commit e permite representar o ROB
// do PDF com slots fixos e colunas de estado visíveis.
class CircularROB {
    public:
        explicit CircularROB(int capacity)
            : slots(capacity), head(0), tail(0), count(0), nextId(1) {
            for (auto& slot : slots) {
                slot.clear();
            }
        }

        bool full() const { return count == (int)slots.size(); }
        bool empty() const { return count == 0; }
        int size() const { return count; }
        int capacity() const { return (int)slots.size(); }
        int headIndex() const { return head; }
        int tailIndex() const { return tail; }

        // allocate: reserva o próximo slot livre e devolve um ponteiro para ele.
        // O slot já sai marcado como busy e com uma tag única (id).
        ROBEntry* allocate() {
            if (full()) return nullptr;

            ROBEntry& slot = slots[tail];
            slot.clear();
            slot.id = nextId++;
            slot.busy = true;
            slot.state = ROBState::Issued;

            int allocatedIndex = tail;
            tail = (tail + 1) % slots.size();
            count++;
            return &slots[allocatedIndex];
        }

        // front: retorna a entrada mais velha ainda ativa.
        ROBEntry* front() {
            if (empty()) return nullptr;
            return &slots[head];
        }

        const ROBEntry* front() const {
            if (empty()) return nullptr;
            return &slots[head];
        }

        // commitFront: remove a entrada mais velha da estrutura circular.
        // Mantemos o conteúdo do slot até sua reutilização para facilitar o log.
        void commitFront() {
            if (empty()) return;
            slots[head].busy = false;
            slots[head].state = ROBState::Commit;
            head = (head + 1) % slots.size();
            count--;
        }

        // findById: localiza uma entrada ativa pelo tag id.
        ROBEntry* findById(int id) {
            for (auto& slot : slots) {
                if (slot.busy && slot.id == id) return &slot;
            }
            return nullptr;
        }

        const ROBEntry* findById(int id) const {
            for (const auto& slot : slots) {
                if (slot.busy && slot.id == id) return &slot;
            }
            return nullptr;
        }

        ROBEntry& slotAt(int index) { return slots[index]; }
        const ROBEntry& slotAt(int index) const { return slots[index]; }

        template <typename Fn>
        void forEachActive(Fn fn) {
            for (int i = 0; i < count; i++) {
                int idx = (head + i) % slots.size();
                if (slots[idx].busy) fn(slots[idx]);
            }
        }

        template <typename Fn>
        void forEachActive(Fn fn) const {
            for (int i = 0; i < count; i++) {
                int idx = (head + i) % slots.size();
                if (slots[idx].busy) fn(slots[idx]);
            }
        }

    private:
        std::vector<ROBEntry> slots;
        int head;
        int tail;
        int count;
        int nextId;
};

#endif
