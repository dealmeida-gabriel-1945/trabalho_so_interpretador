
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void cabecalho();
void despedida();
void trabalha_linha_de_comando(char *linha);

int main() {
#define MAX_STR_LENGTH 256
#define STR_SAIDA "sair\0"
#define STR_PIPE "|"
#define STR_QUEBRA_LINHA "\n"
    //Variável responśavel por armazenar a linha de comando que o usuário digitar
    char linha[MAX_STR_LENGTH] = "";

    //Mostra o cabeçalho do usuario
    cabecalho();

    //rodará infinitamente até o break
    while (1 == 1){

        //printa o sinal $ para melhor decoração
        printf("$ ");
        //recolhe o comando
        scanf("%s", linha);
//        scanf("%[^\n]%*c", linha);

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
    //lista de comandos separadamente
    char  *comandos[256];
    //contador de comandos
    int comandos_cont = 0;

    //separa cada comando através do pipe "|"
    char * token = strtok (linha,STR_PIPE);
    //enquanto tiver mais pipes, irá procurando comandos
    while (token != NULL){
        //armazena o comando na lista de comandos
        comandos[comandos_cont++] = token;
        //busca a proixima string de instrução
        token = strtok (NULL, " \n");
    }
    //adiciona o fim de string
    comandos[comandos_cont] = '\0';

    //todo: execução das intruções com fork e pipe
    for(int i = 0; i<comandos_cont; i++){
        if(strcmp(comandos[i], STR_PIPE) != 0){
            char path[MAX_STR_LENGTH] = "/bin/";
            strcat(path, comandos[i]);
            printf("executar %s\n",path);
            execlp(path, comandos[i], NULL);
        }
    }
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