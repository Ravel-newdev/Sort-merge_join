#ifndef JOIN_H
#define JOIN_H

#include "pagina.h"

//leia com atenção igual o outro

/*sort-merge join
recebe as duas tabelas já ordenadas pela chave de junção
percorre sequencialmente comparando os valores das chaves
quando as chaves batem, gera todas as combinações (produto cartesiano do grupo com mesmo valor)

parametros:
esq: tabela da esquerda (grapes ordenada)
dir: tabela da direita (wines ordenada)
col_esq: nome da coluna de junção em esq ("chave_primaria")
col_dir: nome da coluna de junção em dir ("chave_estrangeira")

retorna a nova tabela com as tuplas resultantes
as tabelas originais não são modificadas*/
Tabela* sort_merge_join(Tabela *esq, Tabela *dir,
                        const char *col_esq, const char *col_dir);

//monta o esquema do resultado da junção
//(todas as colunas da esquerda + todas as colunas da direita)
Esquema montar_esquema_resultado(Esquema *esq, Esquema *dir);

//combina duas tuplas em uma só (pra jogar no resultado)
Tupla combinar_tuplas(Tupla *t_esq, Tupla *t_dir);

//adiciona uma tupla na tabela resultado
//(já gerencia as páginas automaticamente)
void adicionar_tupla(Tabela *resultado, Tupla tupla);

#endif