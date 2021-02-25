//
// Created by gabriel_guimaraes on 2/24/21.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int validaComandoCompleto(char *comando_completo);
int checkFilePath(char *path);
void imprime(char* s);
char** monta_argumentos(char** argumentos, char* comando_completo, int* tamanho);
char** monta_argumentos_sem_redirect(char** argumentos, char** argumentos_sem_redirect, int tamanho, int* tamanho_argumentos_sem_redirect);
char** pega_redirect(char** argumentos, int tamanho, int* tamanho_redirect);

/*COSMETICO*/
void cabecalho();
void marcaPadrao();
void despedida();

/*ERROS*/
void showErro(char *erro);
void showErroDup();
void showErroPath();
void showErroPipe();
void showErroFork();
void showErroRealocacao();
void showErroComandoIndevido();

//=====================================CONSTANTES
/*CONSTANTES NUMÉRICAS*/
#define MAX_STR_LENGTH 256

#define TRUE 1
#define FALSE 0

#define READ_END    0
#define WRITE_END	1

/*CONSTANTES STRINGS*/
#define STR_BREAK_LINE "\n"
#define STR_SPACE " "
#define STR_BIN_PATH "/bin/"
#define STR_HOME_PATH "/home/"

/*CONSTANTES CHAR*/
#define CHAR_BREAK_LINE '\n'
#define CHAR_SPACE ' '
#define CHAR_END_OF_STRING '\0'

#define STR_SAIDA "sair"
#define STR_PIPE "|"
#define STR_ARROW_LEFT "<"
#define STR_ARROW_RIGTH ">"

int main(int argc, char const *argv[]){

    char *comando_completo   = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
    char *comando_path      = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
    char *filepath  = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
    char *buffer    = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
    //verifica se todas as alocações funcionaram
    if((comando_completo == NULL) || (filepath == NULL) || (buffer == NULL) || (comando_path == NULL)){
        showErroRealocacao();
        return 1;
    }

    int fd[2];
    int pid;
    int tamanho_redirect = 0;
    int args_sem_redirect_size;
    int status;
    int tamanho = 0;

    char ch;
    char **argumentos;
    char **argumentos_sem_redirect;
    char **redirect;

    cabecalho();

    while(1 == 1){
        //mostra a marca padrão
        marcaPadrao();
        //recolhe a string
        fgets(comando_completo, MAX_STR_LENGTH, stdin);
        //adiciona o \0 no fim da string
        if(comando_completo[strlen(comando_completo) - 1] == CHAR_BREAK_LINE){
            comando_completo[strlen(comando_completo) - 1] =  CHAR_END_OF_STRING;
        }
        //se a string for a de saída, BREAK!
        if(strcmp(comando_completo, STR_SAIDA) == 0){
            break;
        }
        //verifica se o comando é indevido
        if(validaComandoCompleto(comando_completo) == FALSE){
            showErroComandoIndevido();
        }
        //erro na criação do pipe
        if(pipe(fd) == -1){
            showErroPipe();
            return 1;
        }
        //recolhe os argumentos do comando_completo
        argumentos = monta_argumentos(argumentos, comando_completo, &tamanho);

        /*monta o path para o comando*/
        //primeiro se adiciona o "/bin/"
        strcpy(comando_path, STR_BIN_PATH);
        //depois adiciona o primeiro item do vetor de argumentos, que é o comando
        strcat(comando_path, argumentos[0]);

        redirect = pega_redirect(argumentos, tamanho, &tamanho_redirect);
        //se tiver redirecuionamento
        if (redirect != NULL){
            //monta o vetor de argumentos sem o redirect
            argumentos_sem_redirect = monta_argumentos_sem_redirect(argumentos, argumentos_sem_redirect, tamanho, &args_sem_redirect_size);
            for (int i = 0; i < tamanho_redirect; i++){
                if(strcmp(redirect[i], STR_ARROW_RIGTH) != 0){
                    //realiza o fork
                    pid = fork();
                    if(pid < 0){            //ERRO
                        showErroFork();
                        exit(1);
                    }else if(pid == 0){     //PROCESSO FILHO
                        dup2(fd[WRITE_END], 1);
                        close( fd[READ_END]);
                        execvp(comando_path, argumentos_sem_redirect);
                        showErroDup();
                        exit(1);
                    }else{                  //PROCESSO PAI
                        waitpid(pid, &status, WCONTINUED);
                        close(fd[WRITE_END]);
                    }

                    //realiza o fork
                    pid = fork();
                    if(pid < 0){            //ERRO
                        showErroPipe();
                        exit(1);
                    }else if (pid == 0){   //PROCESSO FILHO
                        close(fd[WRITE_END]);
                        //gerta o path do output file
                        strcpy(filepath, STR_HOME_PATH);
                        strcat(filepath, redirect[i + 1]);

                        //abre arquivo de saída
                        int filedesc = open(filepath, O_CREAT | O_WRONLY);
                        //escreve o arquivo de saída
                        while(read(fd[0], &ch, 1)>0) {
                            write(filedesc, &ch, 1);
                        }
                        //fecha o arquivo de saída
                        close( fd[0]);
                        exit(1);
                    }else{                  //PROCESSO PAI
                        //espera
                        waitpid(pid, &status, WCONTINUED);
                    }
                }else if(strcmp(redirect[i], STR_ARROW_LEFT) != 0){
                    //realiza o fork
                    pid = fork();
                    if(pid < 0){            //ERRO
                        showErroFork();
                        exit(1);
                    }else if(pid == 0){     //PROCESSO
                        strcpy(filepath, STR_HOME_PATH);
                        strcat(filepath, redirect[i + 1]);
                        if(checkFilePath(filepath) == FALSE){
                            showErroPath();
                            exit(1);
                        }

                        int filedesc = open(filepath, O_RDONLY);
                        dup2(filedesc,0);
                        dup2(fd[1],1);
                        while(read(filedesc, &ch, 1)){
                            write(fd[1], &ch, 1);
                        }
                        close(fd[READ_END]);
                        exit(1);
                    }else{                  //PROCESSO PAI
                        //espera
                        waitpid(pid, &status, WCONTINUED);
                        close(fd[WRITE_END]);
                    }

                    //realiza fork
                    pid = fork();
                    if(pid < 0){            //ERRO
                        showErroFork();
                        exit(1);
                    }else if(pid == 0){     //PROCESSO
                        dup2(fd[READ_END],0);
                        read(fd[READ_END], buffer, MAX_STR_LENGTH);
                        argumentos[args_sem_redirect_size+1] = buffer;
                        argumentos[args_sem_redirect_size+2] = NULL;
                        for (int i = 0; i < args_sem_redirect_size; i++){
                            printf("%s\n", argumentos[i]);
                        }
                        execvp(comando_path, argumentos);
                        exit(1);
                    }else{                  //PROCESSO PAI
                        waitpid(pid, &status, WCONTINUED);
                        close(fd[0]);
                    }
                }
            }
        }else{//todo continuar mascaração

            switch( pid = fork()){

                case -1:
                    perror("fork nao pode ser feito");
                    exit(1);
                case 0:

                    close(fd[0]);
                    dup2(fd[1], 1);
                    execvp(comando_path, argumentos);
                    perror("nao pode executar");
                    exit(1);
                default:
                    close(fd[1]);

                    waitpid(pid, NULL, WCONTINUED);

                    while(read(fd[0], &ch, 1) > 0){
                        write(0, &ch, 1);
                    }

            }
        }




    }
    despedida();
    return 0;
}

int validaComandoCompleto(char *comando_completo){
    //se a string tive apenas um \n, é um comando indevido
    if(strcmp(comando_completo, STR_BREAK_LINE) == 0){
        return FALSE;
    }
    //comando válido para ser executado
    return TRUE;
}

int checkFilePath(char *path){
    return fopen(path, "r") ? TRUE : FALSE;
}

char** monta_argumentos_sem_redirect(char** argumentos, char** argumentos_sem_redirect, int tamanho, int* tamanho_argumentos_sem_redirect){
    //pega o ultimo index que possui as arrows
    for (int i = 0; i < tamanho; i++){
        if((strcmp(argumentos[i], STR_ARROW_RIGTH) == 0) || (strcmp(argumentos[i], STR_ARROW_LEFT) == 0)){
            (*tamanho_argumentos_sem_redirect) = i;
        }
    }
    //aloca o vetor de strings
    argumentos_sem_redirect = (char**) malloc((*tamanho_argumentos_sem_redirect) * sizeof(char*));
    if(argumentos_sem_redirect == NULL){
        showErroRealocacao();
        exit(1);
    }
    //aloca as strings
    for (int i = 0; i < (*tamanho_argumentos_sem_redirect); i++){
        argumentos_sem_redirect[i] = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
        if(argumentos_sem_redirect[i] == NULL){
            showErroRealocacao();
            exit(1);
        }
    }
    //copia cada argumento para dentro do vetor
    for (int i = 0; i < (*tamanho_argumentos_sem_redirect); i++){
        strcpy(argumentos_sem_redirect[i], argumentos[i]);
    }

    return argumentos_sem_redirect;
}

char** pega_redirect(char** argumentos, int tamanho, int* tamanho_redirect){

    char** redirecionamento;

    //conta os redirects
    for (int i = 0; i < tamanho; i++){
        if((strcmp(argumentos[i], STR_ARROW_RIGTH) == 0) || (strcmp(argumentos[i], STR_ARROW_LEFT) == 0) ){
            (*tamanho_redirect) += 1;
        }
    }

    if(*tamanho_redirect > 0){
        *tamanho_redirect = 2 * (*tamanho_redirect);
        int j = 0;
        //aloca o tamanho necessário
        redirecionamento = (char**) malloc((*tamanho_redirect) * sizeof(char*));
        if (redirecionamento == NULL){
            showErroRealocacao();
            exit(1);
        }
        //aloca cada parte do vetor redirecionamento
        for (int i = 0; i < (*tamanho_redirect); i++){
            redirecionamento[i] = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
            if(redirecionamento[i] == NULL){
                showErroRealocacao();
                exit(1);
            }
        }
        for (int i = 0; i < tamanho; i++){
            if((strcmp(argumentos[i], STR_ARROW_RIGTH) == 0) || (strcmp(argumentos[i], STR_ARROW_LEFT) == 0) ){
                //copia o conteúdo de argumentos[i] para redirecionamento[j]
                strcpy(redirecionamento[j], argumentos[i]);
                j++;
                strcpy(redirecionamento[j], argumentos[i + 1]);
                j++;
            }
        }

        return redirecionamento;
    }
    return NULL;
}

char** monta_argumentos(char** argumentos, char* comando_completo, int* tamanho){
    //monta uma cópia do comando completo
    char *copia = (char*) malloc(strlen(comando_completo)* sizeof(char));
    //assegura do tamanho do vetor de argumentos iniciar com zero
    (*tamanho) = 0;
    //se der falha na alocação, exit(1)
    if(copia == NULL){
        showErroRealocacao();
        exit(1);
    }
    //copia o conteúdo do comando completo para a cópia
    strcpy(copia, comando_completo);
    //separa por espaços
    char *token = strtok(copia, STR_SPACE);
    while( token != NULL ) {
        token = strtok(NULL, " ");
        (*tamanho) = (*tamanho) + 1;
    }
    //faz um vetor se strings com o tamanho pego anteriormente
    argumentos = (char**) malloc ((*tamanho) * sizeof(char*));
    for(int i = 0; i < (*tamanho); i++){
        argumentos[i] = (char*) malloc(MAX_STR_LENGTH * sizeof(char));
        //verifica alocação
        if(argumentos[i] == NULL){
            showErroRealocacao();
            exit(1);
        }
    }
    //copia, novamente, o comando_completo para dentro da cópia
    strcpy(copia, comando_completo);
    //copia os argumentos
    token = strtok(copia, STR_SPACE);
    int cont = 0;
    while( token != NULL ) {
        strcpy(argumentos[cont], token);
        token = strtok(NULL, STR_SPACE);
        cont++;
    }
    return argumentos;

}

void imprime(char* s){
    printf("%s\n", s);
}

/*COSMETICO*/
void cabecalho(){
    printf("---------------------------------------------------------\n");
    printf("============TRABALHO DE SISTEMAS OPERACIONAIS============\n");
    printf("RESPONSÁVEIS:\n");
    printf("\tDaianne Cynthia Leal, \n\tGabriel Guimarães de Almeida, \n\tRafael Augusto de Rezende Neto\n");//todo inserir nome completo do Rafael
    printf("Para sair digite: sair\n");
    printf("---------------------------------------------------------\n\n");
}
void despedida(){
    printf("\n.....................Stay safe! <3\n");
}
void marcaPadrao(){
    printf("\nಠ_ಠ ");
}

/*ERROS*/
void showErro(char *erro){
    printf("\n#################################################################################\n");
    printf("ERRO: %s", erro);
    printf("\n#################################################################################\n");
}
void showErroRealocacao(){
    showErro("Não foi possível realizar a alocação/realocação.");
}
void showErroPipe(){
    showErro("Não foi possível realizar a criação do pipe.");
}
void showErroFork(){
    showErro("Não foi possível realizar a criação do fork.");
}
void showErroComandoIndevido(){
    showErro("Não foi possível executar o comando digitado.");
}
void showErroDup(){
    showErro("Algo deu errado com o dup.");
}
void showErroPath(){
    showErro("Path inválido.");
}