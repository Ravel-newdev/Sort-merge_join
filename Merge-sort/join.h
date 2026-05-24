#ifndef JOIN_H
#define JOIN_H

#include "pagina.h"
//Leia com atenção igual o outro
/*
  Sort-Merge Join
  Recebe as duas tabelas JÁ ORDENADAS pela chave de junção.
  Percorre sequencialmente comparando os valores das chaves.
  Quando batem, gera todas as combinações (produto cartesiano
  do grupo com mesmo valor de chave).
 
  Parâmetros:
   esq: tabela da esquerda (Grapes ordenada)
   dir: tabela da direita  (Wines ordenada)
   col_esq: nome da coluna de junção em esq ("chave_primaria")
   col_dir: nome da coluna de junção em dir ("chave_estrangeira")
  Retorna nova Tabela com as tuplas resultantes.
  As tabelas originais NÃO são modificadas, obviamente.
*/
Tabela* sort_merge_join(Tabela *esq, Tabela *dir,
                        const char *col_esq, const char *col_dir);
/*
 Monta o esquema do resultado da junção
 (todas as colunas de esq + todas as colunas de dir)
*/
Esquema montar_esquema_resultado(Esquema *esq, Esquema *dir);
/*
 Combina duas tuplas em uma só (para o resultado)
*/
Tupla combinar_tuplas(Tupla *t_esq, Tupla *t_dir);
/*
 Adiciona uma tupla à tabela resultado
 (gerencia páginas automaticamente)
*/
void adicionar_tupla(Tabela *resultado, Tupla tupla);

#endif