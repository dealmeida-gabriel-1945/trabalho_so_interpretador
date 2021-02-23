
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_STR_LENGTH 256
#define STR_SAIDA "sair\n"
#define STR_PIPE "|"
#define STR_SPACE " "
#define CHAR_SPACE ' '
#define STR_QUEBRA_LINHA "\n"
#define CHAR_BREAK_LINE '\n'

#define READ_END	0
#define WRITE_END	1


typedef struct {
    char comando_raw[MAX_STR_LENGTH];
    char parametros[MAX_STR_LENGTH];
}Comando;

typedef struct {
    Comando* comandos;
    long qtd_comandos;
}LinhaDeComando;


void cabecalho();
void despedida();
LinhaDeComando monta_linha_de_comando(char *linha);
Comando monta_comando(char* linha);
void trabalha_linha_de_comando(char *linha);
void removeQuebraLinha(char* str);
void separa_comando_e_parametros(char* str, Comando *toReturn);
char* doFork(Comando *comando, char* resultado);

int main(int argc, char *argv[]) {
    //Variável responśavel por armazenar a linha de comando que o usuário digitar
    char linha[MAX_STR_LENGTH] = "";

    //Mostra o cabeçalho do usuario
    cabecalho();

    //rodará infinitamente até o break
    while (1 == 1){

        //printa o sinal $ para melhor decoração
        printf("$ ");
        //recolhe o comando
        fgets(linha, MAX_STR_LENGTH, stdin);

        //se não for a instrução de saída, executa os comandos
        if(strcmp(linha, "\0") != 0){
            if(strcmp(linha, STR_SAIDA) != 0){
                trabalha_linha_de_comando(linha);
            }else{//se for, break!
                break;
            }
        }
    }
    //mensagem de despedida
    despedida();
    return 0;
}

void trabalha_linha_de_comando(char *linha){
    LinhaDeComando toWork = monta_linha_de_comando(linha);
    //todo realizar os pipes
    for (int i = 0; i < toWork.qtd_comandos; ++i) {

    }
}

LinhaDeComando monta_linha_de_comando(char *linha){
    LinhaDeComando toReturn;
    toReturn.qtd_comandos = 0l;

    toReturn.comandos = malloc(1 * sizeof(Comando));
    //separa cada comando através do pipe "|"
    char * token = strtok (linha,STR_PIPE);
    //enquanto tiver mais pipes, irá procurando comandos
    while (token != NULL){
        if(strcmp(token, STR_PIPE) != 0){
            toReturn.qtd_comandos++;
            toReturn.comandos = realloc(toReturn.comandos, toReturn.qtd_comandos * sizeof(Comando));
            //armazena o comando na lista de comandos
            toReturn.comandos[toReturn.qtd_comandos - 1] = monta_comando(token);
        }

        //busca a proixima string de instrução
        token = strtok (NULL, STR_PIPE);
    }
    return toReturn;
}

Comando monta_comando(char* linha){
    Comando toReturn;
    separa_comando_e_parametros(linha, &toReturn);
    return toReturn;
}

void separa_comando_e_parametros(char* str, Comando *toReturn){
    char comando_raw[strlen(str)];
    char parametros_raw[strlen(str)];
    int i, n = strlen(str);
    int achou_primeiro_espaco = 0, cont_comm = 0, cont_par = 0;
    int inseriu_inicio_do_comando = 0;
    for (i = 0; (i < n); i++){
        if(str[i] != CHAR_BREAK_LINE ){
            if(achou_primeiro_espaco == 0){
                comando_raw[cont_comm++] = str[i];
                if(str[i] != CHAR_SPACE){
                    inseriu_inicio_do_comando = 1;
                }
            }else{
                parametros_raw[cont_par++] = str[i];
            }
            if ((str[i] == CHAR_SPACE) && (inseriu_inicio_do_comando == 1)){
                achou_primeiro_espaco = 1;
            }
        }
    }
    comando_raw[cont_comm] = '\0';
    parametros_raw[cont_par] = '\0';

    strcpy((*toReturn).comando_raw, comando_raw);
    strcpy((*toReturn).parametros, parametros_raw);
}

void removeQuebraLinha(char* str){
    int j, n = strlen(str);
    for (int i = j = 0; i < n; i++)
        if (str[i] != '\n'){
            str[j++] = str[i];
        }

    str[j] = '\0';
}

void cabecalho(){
    printf("---------------------------------------------------------\n");
    printf("============TRABALHO DE SISTEMAS OPERACIONAIS============\n");
    printf("RESPONSÁVEIS:\n");
    printf("\tDaianne Cynthia Leal, \n\tGabriel Guimarães de Almeida, \n\tRafael Neto\n");//todo inserir nome completo do Rafael
    printf("Para sair digite: sair\n");
    printf("---------------------------------------------------------\n\n");
}
void despedida(){
    printf("\n.....................Stay safe! <3\n");
}