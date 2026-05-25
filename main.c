#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pagina.h"
#include "sort.h"
#include "join.h"


Tabela* copiar_n_tuplas(Tabela *original, int limite, const char *nome_tabela) {
    //validação para numeros negativos ou nulos
    if (limite <= 0) {
        return NULL; 
    }

    //sabe quantas tuplas existem no total na tabela original
    int total_tuplas = 0;
    for (int i = 0; i < original->qtd_pags; i++) {
        total_tuplas += original->pags[i]->qtd_tuplas_ocup;
    }

    //validacao para o usuário pedir mais tuplas do que o arquivo tem, ajustar para o máximo atual
    if (limite > total_tuplas) {
        printf("Aviso: %s tem apenas %d tuplas. Usando o limite maximo.\n", nome_tabela, total_tuplas);
        limite = total_tuplas;
    }

    //cria nova tabela em branco para copiar o esquema de colunas
    Tabela *nova = criar_tabela(original->esquema);
    Pagina *pag_nova = criar_pagina();
    int copiadas = 0; //contador de quantas tuplas já foram copiadas

    //laço para copiar as tuplas da tabela original para a nova
    for (int i = 0; i < original->qtd_pags && copiadas < limite; i++) {
        Pagina *p_atual = original->pags[i];
        
        for (int j = 0; j < p_atual->qtd_tuplas_ocup && copiadas < limite; j++) {
            //copia a tupla exata para a nova pagina
            pag_nova->tuplas[pag_nova->qtd_tuplas_ocup++] = p_atual->tuplas[j];
            copiadas++;

            //se a pagina encher (bater 12 tuplas), guarda na tabela e cria outra
            if (pag_nova->qtd_tuplas_ocup == MAX_TUPLAS_PAG) {
                //realloc aumenta o tamanho do vetor de páginas dinamicamente
                nova->pags = realloc(nova->pags, (nova->qtd_pags + 1) * sizeof(Pagina*));
                nova->pags[nova->qtd_pags++] = pag_nova;
                pag_nova = criar_pagina(); //validacao para abrir uma página nova e vazia
            }
        }
    }
    
    // validacao se sobrou alguma página que não encheu totalmente, considera ela tbm
    if (pag_nova->qtd_tuplas_ocup > 0) {
        nova->pags = realloc(nova->pags, (nova->qtd_pags + 1) * sizeof(Pagina*));
        nova->pags[nova->qtd_pags++] = pag_nova;
    } else {
        free(pag_nova); //se a página ta vazia ent libera da memoria
    }
    return nova;
}

int main(void) {
    //ler os dados originais do disco para o esquema inicial
    Tabela *grapes_full = ler_csv("grapes.csv");
    Tabela *wines_full  = ler_csv("wines.csv");

    if (!grapes_full || !wines_full) {
        printf("Erro: Arquivos CSV nao encontrados na pasta\n");
        return 1;
    }

    int opcao;
    int limite_g = 0; //qtdade de grapes para o teste
    int limite_w = 0; // qtdade de wines para o teste

    //menu
    printf("\nMenu Sort-Merge Join\n");
    printf("Teste 1 - Execucao completa (39 Grapes, 511 Wines)\n");
    printf("Teste 2 - Quebra de pagina (ex: 13 Grapes, 13 Wines)\n");
    printf("Teste 3 - Estourando o buffer de memoria (38 Grapes, 100 Wines)\n");
    printf("Teste 4 - Escolher as quantidades manualmente\n");
    printf("Digite a opcao desejada: ");
    
    // validacao para o usuário digitar algo errado, assume a opção 1, por padrão
    if (scanf("%d", &opcao) != 1) opcao = 1;

    //limites com base na escolha
    if (opcao == 1) {
        limite_g = 39; 
        limite_w = 511;
    } else if (opcao == 2) {
        limite_g = 13; 
        limite_w = 13;
    } else if (opcao == 3) {
        limite_g = 39; 
        limite_w = 100; // 100 tuplas garante que vai estourar os B=5 frames (60 tuplas máximo)
    } else if (opcao == 4) {
        printf("Digite a quantidade de Grapes (Ex: 10): ");
        scanf("%d", &limite_g);
        printf("Digite a quantidade de Wines (Ex: 20): ");
        scanf("%d", &limite_w);
        
        // valores que não fazem sentido ter
        if (limite_g <= 0 || limite_w <= 0) {
            printf("Erro: Digite apenas numeros inteiros positivos\n");
            liberar_tabela(grapes_full);
            liberar_tabela(wines_full);
            return 1;
        }
    } else {
        printf("Opcao invalida. Rodando a execucao completa por padrao\n");
        limite_g = 39; 
        limite_w = 511;
    }

    printf("\nDados:\n");
    //cria as tabelas de trabalho com as qtdades certas
    Tabela *grapes = copiar_n_tuplas(grapes_full, limite_g, "Grapes");
    Tabela *wines  = copiar_n_tuplas(wines_full, limite_w, "Wines");

    printf("Tabela Grapes: %d paginas criadas\n", grapes->qtd_pags);
    printf("Tabela Wines:  %d paginas criadas\n", wines->qtd_pags);

    printf("\nOrdenaçao Externa\n");
    //ordena as tabela separadas gerando runs no disco
    Tabela *grapes_ord = sort_externo(grapes, "chave_primaria");
    Tabela *wines_ord  = sort_externo(wines,  "chave_estrangeira");

    printf("Grapes ordenada: %d paginas\n", grapes_ord->qtd_pags);
    printf("Wines ordenada:  %d paginas\n", wines_ord->qtd_pags);

    printf("\nMerge Join\n");
    //junta as duas tabelas ordenadas percorrendo sequencialmente
    Tabela *resultado = sort_merge_join(grapes_ord, wines_ord, "chave_primaria", "chave_estrangeira");

    printf("A Tabela resultado possui: %d paginas\n", resultado->qtd_pags);


    //libera toda a memória alocada para evitar vazamento de memoria
    liberar_tabela(grapes_full);
    liberar_tabela(wines_full);
    liberar_tabela(grapes);
    liberar_tabela(wines);
    liberar_tabela(grapes_ord);
    liberar_tabela(wines_ord);
    liberar_tabela(resultado);

    return 0;
}
