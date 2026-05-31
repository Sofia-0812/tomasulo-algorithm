#ifndef STORE_BUFFER_H
#define STORE_BUFFER_H

// StoreBufferEntry: entrada simples para um buffer de stores separado do ROB.
// O objetivo é representar stores já prontos (endereço+valor) que aguardam
// commit in-order. Mantemos um pequeno buffer fixo para facilitar análise.
// O StoreBuffer é como uma "Sala de espera" dedicada para instruções de Store (SW).
// Como escritas na memória são destrutivas, elas só podem ocorrer na fase de Commit.
// Como a Estação de Reserva do SW é liberada logo após calcular os dados (Write Result),
// este buffer guarda o pacote pronto (endereço + valor) com segurança até a gravação final.
struct StoreBufferEntry {
    int robId = -1;    // id da entrada ROB correspondente
    int addr = 0;      // endereço efetivo calculado
    float value = 0.0f;// valor a ser escrito
    bool ready = false;// true quando endereço+valor estão prontos
    bool busy = false; // se a entrada está ocupada
};

#endif
