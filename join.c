#include "join.h"
#include <string.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────
 * Monta esquema do resultado:
 * todas as colunas de esq + todas de dir
 * ───────────────────────────────────────────── */
Esquema montar_esquema_resultado(Esquema *esq, Esquema *dir) {
    Esquema res;
    res.qtd_cols = 0;

    for (int i = 0; i < esq->qtd_cols; i++)
        strncpy(res.nomes[res.qtd_cols++], esq->nomes[i], MAX_NOME_COL - 1);

    for (int i = 0; i < dir->qtd_cols; i++)
        strncpy(res.nomes[res.qtd_cols++], dir->nomes[i], MAX_NOME_COL - 1);

    return res;
}

/* ─────────────────────────────────────────────
 * Combina duas tuplas em uma só
 * ───────────────────────────────────────────── */
Tupla combinar_tuplas(Tupla *t_esq, Tupla *t_dir) {
    Tupla res;
    res.qtd_cols = 0;

    for (int i = 0; i < t_esq->qtd_cols; i++)
        strncpy(res.cols[res.qtd_cols++], t_esq->cols[i], MAX_COL_TAM - 1);

    for (int i = 0; i < t_dir->qtd_cols; i++)
        strncpy(res.cols[res.qtd_cols++], t_dir->cols[i], MAX_COL_TAM - 1);

    return res;
}

/* ─────────────────────────────────────────────
 * Adiciona tupla à tabela resultado,
 * criando nova página quando necessário
 * ───────────────────────────────────────────── */
void adicionar_tupla(Tabela *resultado, Tupla tupla) {
    /* pega última página ou cria se não existir */
    Pagina *pag = NULL;
    if (resultado->qtd_pags > 0)
        pag = resultado->pags[resultado->qtd_pags - 1];

    if (!pag || pag->qtd_tuplas_ocup == MAX_TUPLAS_PAG) {
        pag = criar_pagina();
        resultado->pags = realloc(resultado->pags,
            (resultado->qtd_pags + 1) * sizeof(Pagina*));
        resultado->pags[resultado->qtd_pags++] = pag;
    }

    pag->tuplas[pag->qtd_tuplas_ocup++] = tupla;
}

/* ─────────────────────────────────────────────
 * SORT-MERGE JOIN
 *
 * Ambas as tabelas já estão ordenadas pela
 * chave de junção. Percorre sequencialmente
 * e gera produto cartesiano dos grupos iguais.
 * ───────────────────────────────────────────── */
Tabela* sort_merge_join(Tabela *esq, Tabela *dir,
                        const char *col_esq, const char *col_dir) {

    Esquema esq_res = montar_esquema_resultado(&esq->esquema, &dir->esquema);
    Tabela *resultado = criar_tabela(esq_res);

    int idx_esq = esquema_indice(&esq->esquema, col_esq);
    int idx_dir = esquema_indice(&dir->esquema, col_dir);

    /* Cursores para percorrer as tabelas */
    int pag_e = 0, tup_e = 0; /* cursor esquerdo  */
    int pag_d = 0, tup_d = 0; /* cursor direito   */

    /* Macro para obter tupla atual de cada lado */
    #define TUP_ESQ (esq->pags[pag_e]->tuplas[tup_e])
    #define TUP_DIR (dir->pags[pag_d]->tuplas[tup_d])

    /* Macro para verificar se cursor chegou ao fim */
    #define FIM_ESQ (pag_e >= esq->qtd_pags)
    #define FIM_DIR (pag_d >= dir->qtd_pags)

    /* Macro para avançar cursor */
    #define AVANCAR_ESQ do { \
        tup_e++; \
        if (tup_e >= esq->pags[pag_e]->qtd_tuplas_ocup) { tup_e = 0; pag_e++; } \
    } while(0)

    #define AVANCAR_DIR do { \
        tup_d++; \
        if (tup_d >= dir->pags[pag_d]->qtd_tuplas_ocup) { tup_d = 0; pag_d++; } \
    } while(0)

    while (!FIM_ESQ && !FIM_DIR) {
        const char *chave_e = TUP_ESQ.cols[idx_esq];
        const char *chave_d = TUP_DIR.cols[idx_dir];
        int cmp = strcmp(chave_e, chave_d);

        if (cmp < 0) {
            /* esquerda menor — avança esquerda */
            AVANCAR_ESQ;

        } else if (cmp > 0) {
            /* direita menor — avança direita */
            AVANCAR_DIR;

        } else {
            /* chaves iguais — coleta grupo da esquerda */
            /* e produto cartesiano com grupo da direita */

            /* salva posição inicial do grupo esquerdo */
            int pag_e_ini = pag_e, tup_e_ini = tup_e;

            /* percorre grupo da direita com mesma chave */
            int pag_d_cur = pag_d, tup_d_cur = tup_d;

            while (!FIM_DIR) {
                const char *chave_d_cur =
                    dir->pags[pag_d_cur]->tuplas[tup_d_cur].cols[idx_dir];
                if (strcmp(chave_d_cur, chave_e) != 0) break;

                /* percorre grupo da esquerda com mesma chave */
                pag_e = pag_e_ini;
                tup_e = tup_e_ini;

                while (!FIM_ESQ) {
                    const char *chave_e_cur = TUP_ESQ.cols[idx_esq];
                    if (strcmp(chave_e_cur, chave_e) != 0) break;

                    Tupla combinada = combinar_tuplas(
                        &TUP_ESQ,
                        &dir->pags[pag_d_cur]->tuplas[tup_d_cur]
                    );
                    adicionar_tupla(resultado, combinada);
                    AVANCAR_ESQ;
                }

                /* avança cursor direito */
                tup_d_cur++;
                if (!FIM_DIR &&
                    tup_d_cur >= dir->pags[pag_d_cur]->qtd_tuplas_ocup) {
                    tup_d_cur = 0;
                    pag_d_cur++;
                }
                if (pag_d_cur >= dir->qtd_pags) break;
            }

            /* avança cursor direito para após o grupo */
            pag_d = pag_d_cur;
            tup_d = tup_d_cur;
        }
    }

    #undef TUP_ESQ
    #undef TUP_DIR
    #undef FIM_ESQ
    #undef FIM_DIR
    #undef AVANCAR_ESQ
    #undef AVANCAR_DIR

    return resultado;
}