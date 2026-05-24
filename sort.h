#ifndef SORT_H
#define SORT_H

#include "pagina.h"
//Leia com atenção as fases por favor 
// Constantes 
#define B               5  
#define FRAMES_ENTRADA  4   
#define FRAMES_SAIDA    1   

/* 
  Fase 1: geração de runs
  Lê B páginas por vez da tabela, ordena na memória pelo campo
  col_junção, grava cada grupo ordenado como uma run em disco.
  Retorna vetor de Tabelas (runs), e atualiza *num_runs.
*/
Tabela** gerar_runs(Tabela *tabela, const char *col_juncao, int *num_runs);
/*
  Fase 2: Merge de runs
  Recebe vetor de runs, intercala usando 4 frames de entrada
  e 1 de saída. Se num_runs > 4, executa múltiplas passadas
  até restar 1 única run ordenada.
  Retorna a Tabela final ordenada.
*/
Tabela* merge_runs(Tabela **runs, int num_runs, const char *col_juncao);
/*
  Função completa: executa fase 1 + fase 2.
  Retorna tabela ordenada pelo campo col_juncao
*/
Tabela* sort_externo(Tabela *tabela, const char *col_juncao);
/*
  Utilitário: compara dois valores de chave (strcmp)
  Retorna <0, 0 ou >0 (igual ao strcmp)
*/
int comparar_chave(const char *a, const char *b);
#endif