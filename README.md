# Simulador Tomasulo

Simulador do algoritmo de Tomasulo em C++ para a disciplina de Arquitetura de Computadores 3.

A implementação modela um processador com execução superescalar (2 vias), execução fora-de-ordem (Out-of-Order Execution) e efetivação em ordem (In-Order Commit) utilizando um Reorder Buffer (ROB).

**Referências:**
1. Computer Architecture : A Quantitative Approach by Hennessy, John L., Patterson, David A.
2. https://github.com/ishank4/Tomasulo-Implementation-in-C 
3. Slides e material didático da disciplina.

# Grupo
1. Arthur Henrique Tristão Pinto
2. Filipi Pereira de Mesquita Faria
3. Julio Cesar Thurow Buzzi
4. Luis Henrique Ferreira Costa
5. Sofia Grossi Vieira Santos

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
132 132.0
244 244.0
116 0.0
```

## Detalhes de Implementação 

1. **Sincronismo Combinacional (Ordem Inversa do Ciclo):**
   Para evitar que um dado produzido no ciclo N seja consumido de forma instantânea em uma etapa posterior dentro do mesmo ciclo N, a função `main` invoca as etapas do pipeline de trás para frente:
   `Commit` -> `Write Result` -> `Execute` -> `Issue` -> `PrintState`.

2. **Despacho Superescalar (Issue):**
   O sistema tenta despachar até 2 instruções simultâneas por ciclo, desde que haja slots físicos disponíveis no ROB e em suas respectivas *Reservation Stations* (evitando *Structural Hazards*). Durante esta etapa, o simulador realiza a **renomeação de registradores** para resolver falsas dependências (WAW e WAR).

3. **Reorder Buffer (ROB) Circular:**
   Implementado na classe `CircularROB`, garante o *In-Order Commit*. As instruções ganham uma Tag única (`nextId`). Se o buffer de tamanho fixo lotar, o despacho é bloqueado estruturalmente até que o `head` libere espaço.

4. **Desambiguação de Memória e Store Forwarding:**
   Na etapa de *Write Result*, instruções `LW` varrem o ROB em busca de instruções `SW` mais antigas.
   - Se houver um `SW` para o mesmo endereço com dado não pronto, o `LW` sofre *Stall*.
   - Se o dado já estiver pronto no ROB, ocorre o *Store Forwarding* (leitura direta sem ir à memória física).

5. **Store Buffer:**
   Implementado separadamente do ROB, reflete a restrição física de que escritas em memória são operações destrutivas e assíncronas. Os pacotes de dados ficam armazenados temporariamente na estrutura e a gravação na RAM só é efetivada na fase final de Commit.

## Estrutura do projeto

- `main.cpp`: Motor do simulador. Contém o loop de clock e as funções principais do pipeline (`issueInstruction`, `executeInstruction`, `writeResult`, `commitInstruction`).
- `Instruction.h`: Estrutura de dados das instruções originais e logs de tempo.
- `ReservationStation.h`: Representação das estações de reserva das unidades funcionais.
- `ROBEntry.h`: Definição de uma entrada individual do ROB (Estado, Destino, Tag).
- `ReorderBuffer.h`: Lógica da fila circular (FIFO) de manipulação do ROB.
- `StoreBuffer.h`: Estrutura isolada para guardar stores até o momento do Commit.
- `instructions.txt` - Arquivo de entrada com as instruções.
- `memory.txt` - Arquivo de memória inicial.
- `result.txt` - Saída gerada pela simulação, para o acompanhamento ciclo a ciclo. 

## Limitações atuais

- O simulador não implementa uma unidade de previsão de desvios. 
- Não há tratamento interno de interrupções de hardware ou exceções (como divisão por zero prolongada).
- O arquivo gerado (`result.txt`) é focado exclusivamente em depuração acadêmica e acompanhamento visual do log didático ciclo a ciclo, imitando as tabelas apresentadas em sala de aula, não em formato final de produção.
