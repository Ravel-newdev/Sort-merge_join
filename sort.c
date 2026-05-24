#include "sort.h"
#include <string.h>
#include <stdlib.h>

/*
  Comparador para qsort — ordena tuplas por
  valor da coluna de índice passado via global.
*/
static int col_juncao_idx = 0; /* índice da coluna de junção */

static int comparar_tuplas(const void *a, const void *b) {
    Tupla *ta = (Tupla*) a;
    Tupla *tb = (Tupla*) b;
    return strcmp(ta->cols[col_juncao_idx], tb->cols[col_juncao_idx]);
}

int comparar_chave(const char *a, const char *b) {
    return strcmp(a, b);
}
 //Adiciona página a uma tabela dinamicamente
static void adicionar_pagina(Tabela *t, Pagina *p) {
    t->pags = realloc(t->pags, (t->qtd_pags + 1) * sizeof(Pagina*));
    t->pags[t->qtd_pags++] = p;
}

/*
  Fase 1: Gera runs
  Lê B páginas por vez, ordena todas as tuplas desse grupo na memória, grava como nova run.
*/
Tabela** gerar_runs(Tabela *tabela, const char *col_juncao, int *num_runs) {
    // Descobre índice da coluna de junção
    col_juncao_idx = esquema_indice(&tabela->esquema, col_juncao);

    int total_pags = tabela->qtd_pags;
    int max_runs   = (total_pags + B - 1) / B; // ceil(N/B)

    Tabela **runs = (Tabela**) malloc(max_runs * sizeof(Tabela*));
    *num_runs = 0;

    int pag_atual = 0;

    while (pag_atual < total_pags) {
        // Carrega até B páginas no buffer
        Tupla buffer[B * MAX_TUPLAS_PAG];
        int   qtd_tuplas = 0;

        for (int f = 0; f < B && pag_atual < total_pags; f++, pag_atual++) {
            Pagina *p = tabela->pags[pag_atual];
            for (int j = 0; j < p->qtd_tuplas_ocup; j++) {
                buffer[qtd_tuplas++] = p->tuplas[j];
            }
        }

        // ── Ordena as tuplas na memória
        qsort(buffer, qtd_tuplas, sizeof(Tupla), comparar_tuplas);

        // Grava run ordenada como nova Tabela
        Tabela *run = criar_tabela(tabela->esquema);
        Pagina *pag = criar_pagina();

        for (int i = 0; i < qtd_tuplas; i++) {
            pag->tuplas[pag->qtd_tuplas_ocup++] = buffer[i];

            if (pag->qtd_tuplas_ocup == MAX_TUPLAS_PAG) {
                adicionar_pagina(run, pag);
                pag = criar_pagina();
            }
        }
        // última página da run (pode estar incompleta)
        if (pag->qtd_tuplas_ocup > 0)
            adicionar_pagina(run, pag);
        else
            free(pag);

        runs[(*num_runs)++] = run;
    }

    return runs;
}

/*
  Fase 2 Merge de runs
  Usa até FRAMES_ENTRADA runs por passada.
  Repete até restar 1 run.
*/
Tabela* merge_runs(Tabela **runs, int num_runs, const char *col_juncao) {

    // Enquanto tiver mais de 1 run, continua intercalando
    while (num_runs > 1) {
        int novas_runs = 0;
        Tabela **resultado = (Tabela**) malloc(
            ((num_runs + FRAMES_ENTRADA - 1) / FRAMES_ENTRADA) * sizeof(Tabela*)
        );

        int i = 0;
        while (i < num_runs) {
            // pega até FRAMES_ENTRADA runs por vez
            int qtd = 0;
            Tabela *grupo[FRAMES_ENTRADA];
            while (qtd < FRAMES_ENTRADA && i < num_runs)
                grupo[qtd++] = runs[i++];

            if (qtd == 1) {
                /* run sozinha — passa direto */
                resultado[novas_runs++] = grupo[0];
                continue;
            }

            // Merge das runs do grupo
            Tabela *merged = criar_tabela(grupo[0]->esquema);
            Pagina *pag_saida = criar_pagina();

            // ponteiros de página e tupla por run
            int pag_ptr[FRAMES_ENTRADA];
            int tup_ptr[FRAMES_ENTRADA];
            for (int r = 0; r < qtd; r++) {
                pag_ptr[r] = 0;
                tup_ptr[r] = 0;
            }

            // obtém índice da coluna de junção
            int idx = esquema_indice(&grupo[0]->esquema, col_juncao);

            while (1) {
                // acha a menor chave entre os FRAMES_ENTRADA cursores
                int menor = -1;
                for (int r = 0; r < qtd; r++) {
                    // verifica se run r ainda tem tuplas
                    if (pag_ptr[r] >= grupo[r]->qtd_pags) continue;
                    Pagina *p = grupo[r]->pags[pag_ptr[r]];
                    if (tup_ptr[r] >= p->qtd_tuplas_ocup)  continue;

                    if (menor == -1) {
                        menor = r;
                    } else {
                        Pagina *pm = grupo[menor]->pags[pag_ptr[menor]];
                        const char *chave_menor = pm->tuplas[tup_ptr[menor]].cols[idx];
                        const char *chave_r     = p->tuplas[tup_ptr[r]].cols[idx];
                        if (strcmp(chave_r, chave_menor) < 0)
                            menor = r;
                    }
                }

                if (menor == -1) break; /* todas as runs esgotadas */

                /* copia tupla do cursor menor para o buffer de saída */
                Pagina *p = grupo[menor]->pags[pag_ptr[menor]];
                pag_saida->tuplas[pag_saida->qtd_tuplas_ocup++] =
                    p->tuplas[tup_ptr[menor]];

                /* avança cursor */
                tup_ptr[menor]++;
                if (tup_ptr[menor] >= p->qtd_tuplas_ocup) {
                    tup_ptr[menor] = 0;
                    pag_ptr[menor]++;
                }

                /* buffer de saída cheio → grava na run merged */
                if (pag_saida->qtd_tuplas_ocup == MAX_TUPLAS_PAG) {
                    adicionar_pagina(merged, pag_saida);
                    pag_saida = criar_pagina();
                }
            }

            /* última página de saída */
            if (pag_saida->qtd_tuplas_ocup > 0)
                adicionar_pagina(merged, pag_saida);
            else
                free(pag_saida);

            /* libera runs do grupo que foram consumidas */
            for (int r = 0; r < qtd; r++)
                liberar_tabela(grupo[r]);

            resultado[novas_runs++] = merged;
        }

        free(runs);
        runs     = resultado;
        num_runs = novas_runs;
    }

    Tabela *final = runs[0];
    free(runs);
    return final;
}

/* ─────────────────────────────────────────────
 * Função principal: sort externo completo
 * ───────────────────────────────────────────── */
Tabela* sort_externo(Tabela *tabela, const char *col_juncao) {
    int num_runs = 0;
    Tabela **runs = gerar_runs(tabela, col_juncao, &num_runs);
    return merge_runs(runs, num_runs, col_juncao);
}