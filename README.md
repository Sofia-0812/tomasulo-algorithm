# Simulador Tomasulo

Simulador do algoritmo de Tomasulo com superescalaridade (2 vias) feito em C++ para a disciplina de Arquitetura de Computadores 3.

## Como compilar

```
g++ -std=c++17 -o tomasulo main.cpp
```

## Como rodar

```
./tomasulo instructions.txt
```

O resultado é salvo no arquivo `result.txt`.

## Formato do arquivo de entrada

Cada linha tem uma instrução. Formato:

Para aritmeticas:
```
ADD F0 F2 F4
SUB F8 F6 F2
MUL F0 F2 F4
DIV F10 F0 F6
```

Para load/store:
```
LW F6 34 R2
SW F6 34 R2
```

## Estrutura do projeto

- `main.cpp` - codigo principal com as 3 fases do tomasulo e o loop de clock
- `Instruction.h` - classe que representa uma instrução
- `ReservationStation.h` - classe que representa uma estação de reserva
- `instructions.txt` - arquivo de entrada com as instrucoes
- `result.txt` - saida gerada pelo simulador

## Como funciona

O simulador executa ciclo a ciclo as 3 fases do algoritmo de Tomasulo:

1. Write Result - se alguma RS terminou de executar, faz broadcast no CDB
2. Execute - se uma RS tem os dois operandos prontos, comeca a executar
3. Issue - pega a proxima instrucao da fila e coloca numa RS livre (ate 2 por ciclo)

### Ordem das fases

A ordem dentro de cada ciclo é Write Result → Execute → Issue. Isso porque se um resultado é escrito no ciclo N, uma instrucao que estava esperando esse operando pode comecar a executar ainda no mesmo ciclo.

### Issue (despacho)

- Pega a proxima instrucao da fila (em ordem do programa)
- Verifica o tipo da operacao e procura uma RS livre do tipo correto
- Se nao tem RS livre, stall (nao despacha nada)
- Preenche os campos da RS:
  - Se o operando ja esta pronto (RegisterStatus vazio) → copia valor do Register File para Vj/Vk
  - Se o operando vai ser produzido por outra RS → coloca o nome da RS em Qj/Qk
- Atualiza o RegisterStatus do registrador destino apontando pra essa RS
- Superescalar: pode despachar ate 2 instrucoes por ciclo

### Execute

- Percorre todas as RS
- Se a RS tem Qj vazio E Qk vazio → operandos prontos → comeca a executar
- Decrementa o contador de ciclos restantes a cada ciclo
- Cada operacao tem uma latencia diferente (ADD=2, MUL=10, etc)

### Write Result (CDB broadcast)

- Quando uma RS termina de executar (ciclos restantes = 0), calcula o resultado
- Percorre TODAS as outras RS: quem tiver Qj ou Qk apontando pra essa RS recebe o valor
- Atualiza o Register File se o RegisterStatus ainda aponta pra essa RS
- Libera a RS
- Apenas 1 resultado por ciclo (1 CDB)

### Resolucao de hazards

- RAW (Read After Write): resolvido pelos campos Qj/Qk. A instrucao espera ate o produtor terminar e fazer broadcast
- WAR (Write After Read): eliminado pela renomeacao implicita. Os valores ja foram copiados pra RS no momento do Issue
- WAW (Write After Write): eliminado pelo RegisterStatus. So a ultima RS a escrever num registrador atualiza o Register File

A saida mostra para cada ciclo:
- Estado das Reservation Stations (Busy, Op, Vj, Vk, Qj, Qk, A)
- Register Status (qual RS vai produzir o valor de cada registrador)
- Register File (valores dos registradores)
- Tabela de instrucoes (em qual ciclo cada fase aconteceu)

## Latencias

- ADD/SUB: 2 ciclos
- MUL: 10 ciclos
- DIV: 40 ciclos
- LW/SW: 2 ciclos

## Reservation Stations

- 3 para ADD/SUB (Add1, Add2, Add3)
- 2 para MUL/DIV (Mul1, Mul2)
- 2 para LOAD (Load1, Load2)
- 2 para STORE (Str1, Str2)
