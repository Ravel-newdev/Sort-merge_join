#include "join.h"
#include <string.h>
#include <stdlib.h>

//junta as colunas das duas tabelas para criar o cabecalho (esquema) do resultado final
Esquema montar_esquema_resultado(Esquema *esq, Esquema *dir) {
    Esquema res;
    res.qtd_cols = 0; //inicia com zero colunas

    //laço para copiar os nomes das colunas da tabela da esquerda //grapes
    for (int i = 0; i < esq->qtd_cols; i++)
        strncpy(res.nomes[res.qtd_cols++], esq->nomes[i], MAX_NOME_COL - 1);

    //laço para copiar os nomes das colunas da tabela da direita //wines
    for (int i = 0; i < dir->qtd_cols; i++)
        strncpy(res.nomes[res.qtd_cols++], dir->nomes[i], MAX_NOME_COL - 1);

    return res;
}

//pega os dados de uma tupla da esquerda e uma da direita e forma uma linha só
Tupla combinar_tuplas(Tupla *t_esq, Tupla *t_dir) {
    Tupla res;
    res.qtd_cols = 0; //inicia a nova tupla vazia

    //copia os valores da tupla da esquerda (dados das grapes)
    for (int i = 0; i < t_esq->qtd_cols; i++)
        strncpy(res.cols[res.qtd_cols++], t_esq->cols[i], MAX_COL_TAM - 1);

    //continua copiando os valores da tupla da direita (dados das wines)
    for (int i = 0; i < t_dir->qtd_cols; i++)
        strncpy(res.cols[res.qtd_cols++], t_dir->cols[i], MAX_COL_TAM - 1);

    return res;
}

//coloca a tupla na tabela de resultado e cuida da paginacao dinamicamente
void adicionar_tupla(Tabela *resultado, Tupla tupla) {
    Pagina *pag = NULL;
    
    //se a tabela ja tem paginas, pega a ultima para tentar preencher
    if (resultado->qtd_pags > 0)
        pag = resultado->pags[resultado->qtd_pags - 1];

    //se nao tem pagina OU se a ultima pagina ta cheia (bateu 12 tuplas)
    if (!pag || pag->qtd_tuplas_ocup == MAX_TUPLAS_PAG) {
        pag = criar_pagina(); //abre uma pagina nova e vazia
        
        //aumenta o espaco do vetor de paginas (realloc) para caber a nova pagina
        resultado->pags = realloc(resultado->pags, (resultado->qtd_pags + 1) * sizeof(Pagina*));
        
        //salva a nova pagina no final do vetor e aumenta o contador de paginas
        resultado->pags[resultado->qtd_pags++] = pag;
    }

    //adiciona a tupla combinada na pagina e aumenta o contador de espaco ocupado dela
    pag->tuplas[pag->qtd_tuplas_ocup++] = tupla;
}

//recebe tabelas ordenadas, percorre sequencialmente e faz a juncao (funcao principal)
Tabela* sort_merge_join(Tabela *esq, Tabela *dir, const char *col_esq, const char *col_dir) {

    //cria a tabela final vazia usando a funcao auxiliar de esquema ali de cima
    Esquema esq_res = montar_esquema_resultado(&esq->esquema, &dir->esquema);
    Tabela *resultado = criar_tabela(esq_res);

    //descobre em qual numero de coluna estao as chaves (ex: coluna 0 = chave_primaria)
    int idx_esq = esquema_indice(&esq->esquema, col_esq); 
    int idx_dir = esquema_indice(&dir->esquema, col_dir); 

    //cursores da esquerda significa em qual pagina e em qual linha da uva estamos agora
    int pag_e = 0, tup_e = 0; 
    //cursores da direita significa em qual pagina e em qual linha do vinho estamos agora
    int pag_d = 0, tup_d = 0; 

    // MACROS: atalhos para o codigo nao ficar poluido
    // TUP_ESQ e TUP_DIR pegam exatamente o texto da linha onde o cursor ta apontando
    #define TUP_ESQ (esq->pags[pag_e]->tuplas[tup_e])
    #define TUP_DIR (dir->pags[pag_d]->tuplas[tup_d])

    //verifica se os cursores de pagina ja passaram do limite de paginas da tabela (Fim do arquivo)
    #define FIM_ESQ (pag_e >= esq->qtd_pags)
    #define FIM_DIR (pag_d >= dir->qtd_pags)

    //AVANCAR_ESQ: Pula pra proxima linha. Se a linha passar de 12, zera a linha e pula pra proxima pagina
    #define AVANCAR_ESQ do { \
        tup_e++; \
        if (tup_e >= esq->pags[pag_e]->qtd_tuplas_ocup) { tup_e = 0; pag_e++; } \
    } while(0)

    //AVANCAR_DIR: Faz exatamente a mesma coisa, mas para os cursores do vinho
    #define AVANCAR_DIR do { \
        tup_d++; \
        if (tup_d >= dir->pags[pag_d]->qtd_tuplas_ocup) { tup_d = 0; pag_d++; } \
    } while(0)

    //laco principal do Sort-Merge (Roda enquanto NENHUM dos arquivos tiver chegado no fim)
    while (!FIM_ESQ && !FIM_DIR) {
        
        //pega as duas chaves que os cursores estao apontando agora
        const char *chave_e = TUP_ESQ.cols[idx_esq];
        const char *chave_d = TUP_DIR.cols[idx_dir];
        
        //strcmp compara alfabeticamente: < 0 (esq menor), > 0 (dir menor), == 0 (iguais)
        int cmp = strcmp(chave_e, chave_d);

        if (cmp < 0) {
            //chave da uva vem antes do vinho no alfabeto e como ta ordenado, nunca vai achar par, entao avanca uva
            AVANCAR_ESQ;

        } else if (cmp > 0) {
            //chave do vinho vem antes da uva no alfabeto. Avanca o vinho pra tentar alcancar a uva
            AVANCAR_DIR;

        } else {
            //CMP == 0 significa chaves iguais, gera o produto cartesiano (relacao N:M)

            //salva EXATAMENTE onde o grupo repetido da esquerda comecou (o "marcador")
            int pag_e_ini = pag_e, tup_e_ini = tup_e;

            //cria cursores temporarios para passear pelo grupo repetido da direita
            int pag_d_cur = pag_d, tup_d_cur = tup_d;

            //laco que percorre todos os vinhos que tem essa exata mesma chave
            while (!FIM_DIR) {
                //pega a chave do vinho temporario atual
                const char *chave_d_cur = dir->pags[pag_d_cur]->tuplas[tup_d_cur].cols[idx_dir];
                
                //se a chave mudou, o grupo repetido de vinhos acabou, entao quebra o laco
                if (strcmp(chave_d_cur, chave_e) != 0) break;

                //para cruzar ESSE vinho atual com TODAS as uvas, volta o cursor da uva pro marcador
                pag_e = pag_e_ini;
                tup_e = tup_e_ini;

                //laco que percorre todas as uvas que tem essa exata mesma chave
                while (!FIM_ESQ) {
                    const char *chave_e_cur = TUP_ESQ.cols[idx_esq];
                    
                    //see a uva mudou de chave, esse grupinho acabou, quebra o laco
                    if (strcmp(chave_e_cur, chave_e) != 0) break; 

                    //junta a uva repetida atual com o vinho repetido atual
                    Tupla combinada = combinar_tuplas(
                        &TUP_ESQ,
                        &dir->pags[pag_d_cur]->tuplas[tup_d_cur]
                    );
                    
                    //joga a combinacao na tabela resultado (o adicionar_tupla cuida da memoria)
                    adicionar_tupla(resultado, combinada);
                    
                    //avanca a uva para cruzar o proximo membro do grupo com este mesmo vinho
                    AVANCAR_ESQ;
                }

                //Cruzado todas as uvas com o vinho atual, avanca o vinho pro proximo repetido
                tup_d_cur++;
                
                //logica manual para pular a pagina temporaria se a linha passar de 12
                if (!FIM_DIR && tup_d_cur >= dir->pags[pag_d_cur]->qtd_tuplas_ocup) {
                    tup_d_cur = 0;
                    pag_d_cur++;
                }
                
                //se no meio desse avanco o arquivo acabou, quebra o laco de seguranca
                if (pag_d_cur >= dir->qtd_pags) break;
            }

            //terminando as multiplicacoes. o cursor principal do vinho pro ponto onde o grupo acabou é atualizado
            pag_d = pag_d_cur;
            tup_d = tup_d_cur;
            
            //ocursor da uva (pag_e) ja foi avancado automaticamente ate o fim do grupo no laco interno
        }
    }

    //remove as macros pra nao dar conflito
    #undef TUP_ESQ
    #undef TUP_DIR
    #undef FIM_ESQ
    #undef FIM_DIR
    #undef AVANCAR_ESQ
    #undef AVANCAR_DIR

    //retorna a tabela completa com as quebras de pagina feitas
    return resultado;
}