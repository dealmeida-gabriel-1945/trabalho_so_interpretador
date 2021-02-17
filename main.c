
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void cabecalho();
void despedida();

int main() {
#define MAX_STR_LENGTH 255
#define STR_SAIDA "sair\0"
    //linha de comando
    char linha[MAX_STR_LENGTH] = "estepe";
    //comandos separados
    char **comandos;
    int tam_com = 1, qtd_com = 0;
    comandos = calloc(tam_com, sizeof (char*));

    cabecalho();

    while (strcmp(linha, STR_SAIDA) != 0){
        printf("$");
        scanf("%[^\n]%*c", linha);
    }
    despedida();
    return 0;
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