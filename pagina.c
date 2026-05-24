#include "pagina.h"

/*
  Retorna índice da coluna pelo nome.
  Retorna -1 se não encontrar.
 */
int esquema_indice(Esquema *esq, const char *nome_col) {
    for (int i = 0; i < esq->qtd_cols; i++) {
        if (strcmp(esq->nomes[i], nome_col) == 0)
            return i;
    }
    return -1;
}
//Cria uma página vazia
Pagina* criar_pagina() {
    Pagina *p = (Pagina*) malloc(sizeof(Pagina));
    p->qtd_tuplas_ocup = 0;
    return p;
}

 //Cria tabela vazia com esquema dado
Tabela* criar_tabela(Esquema esq) {
    Tabela *t = (Tabela*) malloc(sizeof(Tabela));
    t->pags     = NULL;
    t->qtd_pags = 0;
    t->esquema  = esq;
    return t;
}
//Libera toda a memória de uma tabela
void liberar_tabela(Tabela *t) {
    if (!t) return;
    for (int i = 0; i < t->qtd_pags; i++)
        free(t->pags[i]);
    free(t->pags);
    free(t);
}

/*
  Parser de linha CSV respeitando aspas.
  Preenche tupla com os campos encontrados.
  Campos entre aspas podem conter vírgulas.
 */
static void parsear_linha_csv(char *linha, Tupla *tupla, int qtd_cols) {
    tupla->qtd_cols = 0;
    int col = 0;
    char *p = linha;

    while (*p && col < qtd_cols) {
        char *dest = tupla->cols[col];
        int pos    = 0;

        if (*p == '"') {
            // campo entre aspas, pode ter vírgulas dentro
            p++; // pula a aspas de abertura 
            while (*p) {
                if (*p == '"' && *(p+1) == '"') {
                    /* aspas escapada ("") */
                    dest[pos++] = '"';
                    p += 2;
                } else if (*p == '"') {
                    p++; /* aspas de fechamento */
                    break;
                } else {
                    dest[pos++] = *p++;
                }
                if (pos >= MAX_COL_TAM - 1) break;
            }
        } else {
            // campo simples sem aspas
            while (*p && *p != ',' && *p != '\n' && *p != '\r') {
                dest[pos++] = *p++;
                if (pos >= MAX_COL_TAM - 1) break;
            }
        }

        dest[pos] = '\0';
        col++;
        tupla->qtd_cols++;

        // avança a vírgula separadora
        if (*p == ',') p++;
    }
}

/*
  Lê CSV e monta a Tabela.
  Primeira linha = cabeçalho (esquema).
  Cada grupo de 12 tuplas = 1 página.
*/
Tabela* ler_csv(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (!f) {
        fprintf(stderr, "Erro: não foi possível abrir %s\n", caminho);
        return NULL;
    }

    char linha[MAX_COLS * MAX_COL_TAM];

    // Lê cabeçalho. aí monta esquema
    if (!fgets(linha, sizeof(linha), f)) {
        fclose(f);
        return NULL;
    }
    linha[strcspn(linha, "\r\n")] = '\0';

    Esquema esq;
    esq.qtd_cols = 0;
    // reutiliza o parser mas para o esquema
    Tupla cab;
    parsear_linha_csv(linha, &cab, MAX_COLS);
    esq.qtd_cols = cab.qtd_cols;
    for (int i = 0; i < cab.qtd_cols; i++)
        strncpy(esq.nomes[i], cab.cols[i], MAX_NOME_COL - 1);

    Tabela *t = criar_tabela(esq);

    // Lê tuplas linha a linha
    Pagina *pag_atual = criar_pagina();

    while (fgets(linha, sizeof(linha), f)) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (strlen(linha) == 0) continue;

        Tupla tupla;
        parsear_linha_csv(linha, &tupla, esq.qtd_cols);
        tupla.qtd_cols = esq.qtd_cols;

        // adiciona tupla na página atual
        pag_atual->tuplas[pag_atual->qtd_tuplas_ocup++] = tupla;

        // página cheia, então adiciona à tabela e abre nova
        if (pag_atual->qtd_tuplas_ocup == MAX_TUPLAS_PAG) {
            t->pags = realloc(t->pags, (t->qtd_pags + 1) * sizeof(Pagina*));
            t->pags[t->qtd_pags++] = pag_atual;
            pag_atual = criar_pagina();
        }
    }

    // última página (pode estar incompleta)
    if (pag_atual->qtd_tuplas_ocup > 0) {
        t->pags = realloc(t->pags, (t->qtd_pags + 1) * sizeof(Pagina*));
        t->pags[t->qtd_pags++] = pag_atual;
    } else {
        free(pag_atual);
    }

    fclose(f);
    return t;
}
 // Imprime tabela no terminal (debug)
void imprimir_tabela(Tabela *t) {
    // cabeçalho 
    for (int c = 0; c < t->esquema.qtd_cols; c++) {
        printf("%s", t->esquema.nomes[c]);
        if (c < t->esquema.qtd_cols - 1) printf(" | ");
    }
    printf("\n");
    for (int c = 0; c < t->esquema.qtd_cols * 20; c++) printf("-");
    printf("\n");

    // tuplas
    for (int i = 0; i < t->qtd_pags; i++) {
        Pagina *p = t->pags[i];
        for (int j = 0; j < p->qtd_tuplas_ocup; j++) {
            Tupla *tup = &p->tuplas[j];
            for (int c = 0; c < tup->qtd_cols; c++) {
                printf("%.30s", tup->cols[c]);
                if (c < tup->qtd_cols - 1) printf(" | ");
            }
            printf("\n");
        }
    }
    printf("Total: %d páginas\n", t->qtd_pags);
}
