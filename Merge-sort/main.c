#include <stdio.h>
#include <stdlib.h>
#include "pagina.h"
#include "sort.h"
#include "join.h"

int main(void) {

    //Lê os CSVs do trabalho
    Tabela *grapes = ler_csv("grapes.csv");
    Tabela *wines  = ler_csv("wines.csv");

    if (!grapes) { fprintf(stderr, "Erro ao ler grapes.csv\n"); return 1; }
    if (!wines)  { fprintf(stderr, "Erro ao ler wines.csv\n");  return 1; }

    printf("Grapes: %d páginas\n", grapes->qtd_pags);
    printf("Wines:  %d páginas\n", wines->qtd_pags);

    //Sort externo de cada tabela
    Tabela *grapes_ord = sort_externo(grapes, "chave_primaria");
    Tabela *wines_ord  = sort_externo(wines,  "chave_estrangeira");

    printf("Grapes ordenado: %d páginas\n", grapes_ord->qtd_pags);
    printf("Wines ordenado:  %d páginas\n", wines_ord->qtd_pags);

    // Sort-Merge Join
    Tabela *resultado = sort_merge_join(grapes_ord, wines_ord,
                                        "chave_primaria",
                                        "chave_estrangeira");

    printf("Resultado: %d páginas\n", resultado->qtd_pags);

    //Imprime resultado
    imprimir_tabela(resultado);

    // Libera memória
    liberar_tabela(grapes);
    liberar_tabela(wines);
    liberar_tabela(grapes_ord);
    liberar_tabela(wines_ord);
    liberar_tabela(resultado);

    return 0;
}