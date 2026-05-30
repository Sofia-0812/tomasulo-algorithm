# Simulador Tomasulo

Simulador do algoritmo de Tomasulo em C++ para a disciplina de Arquitetura de Computadores 3.

## Como compilar

```bash
clang++ -std=c++11 -g main.cpp -o tomasulo
```

## Como rodar

```bash
./tomasulo instructions.txt [memory.txt]
```

- `instructions.txt` é o arquivo de instruções.
- `memory.txt` é opcional e inicializa a memória antes da simulação.
- A saída é gravada em `result.txt`.

## Formato dos arquivos

### `instructions.txt`

Uma instrução por linha.

Exemplos:

```text
ADD F0 F2 F4
SUB F8 F6 F2
MUL F0 F2 F4
DIV F10 F0 F6
LW F6 32 R2
SW F6 16 R2
```

### `memory.txt`

Cada linha deve ter:

```text
<endereco> <valor>
```

Exemplo:

```text
# comentario ignorado
132 132.0
244 244.0
116 0.0
```

Linhas que começam com `#` são ignoradas.

## O que está implementado

- Issue superscalar com até 2 instruções por ciclo.
- Reservation Stations para `ADD/SUB`, `MUL/DIV`, `LOAD` e `STORE`.
- Register Status para renomeação de registradores.
- CDB lógico para broadcast dos resultados escritos.
- ROB circular com capacidade fixa, `head/tail` e tags únicas.
- Estado explícito do ROB na saída (`Issue`, `Execute`, `Write result`, `Commit`).
- Commit in-order via ROB.
- Tabela de status das instruções com ciclos de `Issue`, `Exec Ini`, `Exec Fim`, `Write` e `Commit`.
- Modelo de memória com endereços inteiros e valores `float`.
- `LW` com cálculo de endereço efetivo.
- `SW` com cálculo de endereço efetivo e escrita em memória no commit.
- Forwarding e desambiguação de memória para `LW` quando há stores anteriores no ROB.
- Store Buffer separado para representar stores prontas antes do commit.
- Saída detalhada em `result.txt` para acompanhamento ciclo a ciclo.

## O que foi mapeado do PDF

- Reorder Buffer.
- Reservation Stations.
- Register Status / renomeação de registradores.
- Pipeline de issue, execute, write result e commit.
- Load/Store com desambiguação de memória.

## Limitações atuais

- Não há branch prediction nem execução especulativa com rollback.
- Não há tratamento de exceções/interrupts.
- O simulador não implementa uma política avançada de predição de desvios.
- O `result.txt` é focado em depuração e aprendizado, não em formato final de produção.

## Estrutura do projeto

- `main.cpp` - simulador principal e loop de clock.
- `Instruction.h` - representação da instrução e seus ciclos.
- `ReservationStation.h` - estrutura da estação de reserva.
- `ROBEntry.h` - entrada do ROB e estado explícito.
- `ReorderBuffer.h` - implementação circular do ROB.
- `StoreBuffer.h` - buffer separado para stores prontas.
- `instructions.txt` - arquivo de entrada de exemplo.
- `memory.txt` - arquivo opcional de memória inicial.
- `result.txt` - saída gerada pela simulação.

## Observações de funcionamento

- Ordem por ciclo: `Write Result` → `Execute` → `Issue` → `Print`.
- `ADD/SUB` usam latência curta; `MUL/DIV` usam latência maior; `LW/SW` usam latência de memória.
- `LW` e `SW` respeitam dependências via ROB e registradores renomeados.
- O simulador foi organizado em arquivos menores para facilitar leitura e estudo.
