 #ifndef PAGINA_H
#define PAGINA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constantes 
#define MAX_TUPLAS_PAG  12    
#define MAX_COLS        10    
#define MAX_COL_TAM    512    
#define MAX_NOME_COL    64    

//  Tupla
// Uma linha da tabela = vetor de strings
typedef struct {
    char cols[MAX_COLS][MAX_COL_TAM];
    int  qtd_cols;
} Tupla;

// Página
// Unidade de I/O — até 12 tuplas
typedef struct {
    Tupla tuplas[MAX_TUPLAS_PAG];
    int   qtd_tuplas_ocup;
} Pagina;

// Esquema
// Aqui vou guardar os nomes das colunas e seus índices 
typedef struct {
    int  qtd_cols;
    char nomes[MAX_COLS][MAX_NOME_COL];
} Esquema;

// Tabela
// Conjunto de páginas + esquema
typedef struct {
    Pagina  **pags;       
    int       qtd_pags;   
    Esquema   esquema;    
} Tabela;

int esquema_indice(Esquema *esq, const char *nome_col);
Pagina* criar_pagina();
Tabela* criar_tabela(Esquema esq);
void liberar_tabela(Tabela *t);
Tabela* ler_csv(const char *caminho);
void imprimir_tabela(Tabela *t);

#endif