# Sort-Merge Join — Trabalho 3 SGBD

## Descrição

Implementação do operador de junção **Sort-Merge Join** em C,
aplicado sobre as tabelas Grapes e Wines. O operador realiza:

1. **Sort externo** de cada tabela com buffer de B=5 frames
2. **Merge das runs** com 4 frames de entrada + 1 de saída
3. **Merge Join** percorrendo sequencialmente as tabelas ordenadas

## Estrutura do projeto
.
├── main.c      # Ponto de entrada
├── pagina.h    # Estruturas: Tupla, Pagina, Tabela, Esquema
├── pagina.c    # Leitura de CSV, criação e liberação de tabelas
├── sort.h      # Protótipos do sort externo
├── sort.c      # Geração de runs + merge de runs
├── join.h      # Protótipos do Sort-Merge Join
├── join.c      # Implementação do join
├── grapes.csv  # Tabela Grapes (38 tuplas)
├── wines.csv   # Tabela Wines (510 tuplas)
└── Makefile

## Como compilar

```bash
make
```

Ou manualmente:

```bash
gcc -Wall -g -o smj main.c pagina.c sort.c join.c
```

## Como executar

```bash
./smj
```

O resultado é impresso no terminal com todas as tuplas
resultantes da junção entre Grapes e Wines pela chave
`chave_primaria = chave_estrangeira`.

## Detalhes de implementação

- **Buffer**: B=5 frames, cada frame = 1 página = 12 tuplas
- **Geração de runs**: lê 5 páginas por vez, ordena com qsort,
  grava como run
- **Merge de runs**: usa até 4 frames de entrada + 1 de saída.
  Se houver mais de 4 runs, executa múltiplas passadas
- **Join**: produto cartesiano para grupos com mesma chave
- As tabelas originais **não são modificadas**
- Apenas **1 página** é carregada por frame em cada momento