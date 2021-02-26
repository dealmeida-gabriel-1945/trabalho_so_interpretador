#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

/*===== CONSTANTES =====*/
//NÚMEROS
#define LIMITE 256              // Número máximo de tokens por comando
#define MAX_STR_LENGTH 1024     // Quantidade máxima de caracteres da linha de comando inserida
#define READ_END    0
#define WRITE_END	1

//STRINGS
#define STR_SAIDA "sair\n"
#define STR_VAZIA " \n\t"
#define STR_LEFT_ARROW "<"
#define STR_RIGTH_ARROW ">"
#define STR_E_COMERCIAL "&"
#define STR_PIPE "|"

//LÓGICOS
#define TRUE 1          //Verdadeiro <-> true
#define FALSE !TRUE     //Falso      <-> false

/*===== SUBROTINAS =====*/
//DECORAÇÃO
void cabecalho();
void prefixo();
void despedida();
//LÓGICA
int trataComando(char* tokens[]);
void trataPipe(char* tokens[]);
int verificaQuantidadeComandos(char *tokens[]);
void trataTicket(char * tokens[], char* input, char* output, int opcoes);
void launch(char** tokens);
//ERROS
void showErro(char* toShow);
void showErroDefault();
void showErroExecutarFork();
//SECRETS
void trataSecret(char* linha);

pid_t pid;


int main(int argc, char *argv[], char ** envp) {
    char linha_comando[MAX_STR_LENGTH];     //Responsável por guardar o que o usuário digitar
    char* tokens[LIMITE];                   //Array para armazenar os tokens da linha e comando
    int qtdTokens;                          //Quantidade de tokens (controle do array)

    //Valor estepe
    pid = -3;
    //Chama o cabeçalho
    cabecalho();

    //Loop de recolhimento dos comandos
    while(1 == 1){
        //Esvazia a string
        linha_comando[0] = '\0';

        //Mostra o prefixo
        prefixo();

        //Recolhe o que o usuário digitar
        fgets(linha_comando, MAX_STR_LENGTH, stdin);

        //SECRETS XD
        trataSecret(linha_comando);

        //Se for a ordem de saída, quebra o laço
        if(strcmp(linha_comando, STR_SAIDA) == 0) break;

        //strtok para verificar se foi escrito algo
        char * token = strtok(linha_comando, STR_VAZIA);
        tokens[0] = token;

        if((strcmp(linha_comando, "\n")) != 0 && (tokens[0] != NULL)){
            //Coleta os tokens
            qtdTokens = 1;
            //Executa enquanto o srtok não retornar NULL (short way de realizar strtok)
            while((tokens[qtdTokens] = strtok(NULL, STR_VAZIA)) != NULL) qtdTokens++;

            trataComando(tokens);
        }
    }
    //Mostra a mensagem e despedida
    despedida();
    exit(0);
}

/*===== SUBROTINAS =====*/
//Decoração
void cabecalho(){
    printf("---------------------------------------------------------\n");
    printf("============TRABALHO DE SISTEMAS OPERACIONAIS============\n");
    printf("RESPONSÁVEIS:\n");
    printf("\tDaianne Cynthia Leal, \n\tGabriel Guimarães de Almeida, \n\tRafael Neto\n");//todo inserir nome completo do Rafael
    printf("Para sair digite: sair\n");
    printf("---------------------------------------------------------\n\n");
}
void despedida(){
    printf("\n.....................Stay safe! ᕕ( ᐛ )ᕗ\n");
}
void prefixo(){
    printf(" ᕕ(ಠ_ಠ)--☞ ");
}

//Logica
int trataComando(char* tokens[]){
    int i = 0, j = 0, descritorDeArquivo, out, aux;
    char *tokens_aux[LIMITE];

    //Separa sinais dos comandos (sinais: &, >, <)
    while (tokens[j] != NULL){
        //Se achar algum dos sinais, pare e continue a executar
        if ((strcmp(tokens[j], STR_RIGTH_ARROW) == 0) || (strcmp(tokens[j], STR_LEFT_ARROW) == 0) || (strcmp(tokens[j], STR_E_COMERCIAL) == 0)){
            break;
        }
        //Adiciona ao auxiliar os valores que não são sinais
        tokens_aux[j] = tokens[j];
        j++;
    }

    //Aqui o auxiliar dos tokens terá apenas script, sem os sinais
    //Aqui, também, será executado os comandos
    while (tokens[i] != NULL){
        //Se o comando for "|"
        if (strcmp(tokens[i], STR_PIPE) == 0){
            trataPipe(tokens);
            return 1;

        //Se o comando for "<"
        }else if (strcmp(tokens[i], STR_LEFT_ARROW) == 0){
            aux = i+1;
            trataTicket(tokens_aux, tokens[i+1], tokens[i+3],1);
            return 1;

        //Se o comando for ">"
        }else if (strcmp(tokens[i], STR_RIGTH_ARROW) == 0){
            trataTicket(tokens_aux, NULL, tokens[i+1], 0);
            return 1;
        }
        i++;
    }
    //Executa o resto dos tokens
    tokens_aux[i] = NULL;
    launch(tokens_aux);

    return 1;
}
void trataPipe(char * tokens[]){
    //Variáveis dos laços de repetição
    int loop_1 = 0;
    int loop_2 = 0;
    int loop_3 = 0;
    //Tickets
    /*
     * 0 READ (READ_END)
     * 1 WRITE (WRITE_END)
     * */
    int des_p_1[2], des_p_2[2];
    //Controles
    int qtdComandos = verificaQuantidadeComandos(tokens);
    int finalizar = 0;
    //Vetor comandos auxiliar
    char *comandos[LIMITE];
    //Objeto do PId
    pid_t pid;


    //Laço que executará os pipes
    while ((tokens[loop_2] != NULL) && (finalizar != 1)){
        loop_3 = 0;
        while (strcmp(tokens[loop_2], STR_PIPE) != 0){
            comandos[loop_3] = tokens[loop_2];
            loop_2++;
            //Se não há mais argumentos, finalize
            if (tokens[loop_2] == NULL){
                loop_3++;
                finalizar = 1;
                break;
            }
            loop_3++;
        }
        //Posição final para indicar a passagem pro exec
        comandos[loop_3] = NULL;
        loop_2++;

        /*
         *  Pipe sendo compartilhado entre iterações para tornar possíveis as operações de outputs e inputs
         * entre dois comandos.
        */
        pipe(((loop_1 % 2) != 0) ? des_p_1 : des_p_2);
        //Realiza o fork
        pid = fork();

        //Erro ao criar o fork
        if(pid == -1){
            showErroExecutarFork();
            close(((loop_1 % 2) != 0) ? des_p_1[WRITE_END] : des_p_2[WRITE_END]);
            return;
        }
        //Filho
        if(pid == 0){
            //Realiza o dup no primeiro comando
            if (loop_1 == 0){
                dup2(des_p_2[WRITE_END], STDOUT_FILENO);
            //Realiza dup no último comando
            }else if (loop_1 == (qtdComandos - 1)){
                //Faz o dup2
                dup2(((qtdComandos % 2) != 0) ? des_p_1[READ_END] : des_p_2[READ_END], STDIN_FILENO);
            //Realiza dup no comando do meio
            }else{
                if ((loop_1 % 2) != 0){
                    dup2(des_p_2[READ_END],STDIN_FILENO);
                    dup2(des_p_1[WRITE_END],STDOUT_FILENO);
                }else{ // for even i
                    dup2(des_p_1[READ_END],STDIN_FILENO);
                    dup2(des_p_2[WRITE_END],STDOUT_FILENO);
                }
            }
            //Executa o comando
            if (execvp(comandos[0], comandos) == -1){
                kill(getpid(), SIGTERM);
            }
        }

        //Fechando os tickets
        //Do inicio
        if (loop_1 == 0){
            close(des_p_2[WRITE_END]);
        //Do meio
        } else if (loop_1 == qtdComandos - 1){
            if ((qtdComandos % 2) != 0){
                close(des_p_1[READ_END]);
            }else{
                close(des_p_2[READ_END]);
            }
        //Do processo final
        }else{
            if (loop_1 % 2 != 0){
                close(des_p_2[READ_END]);
                close(des_p_1[WRITE_END]);
            }else{
                close(des_p_1[READ_END]);
                close(des_p_2[WRITE_END]);
            }
        }
        //Espera
        waitpid(pid,NULL,0);
        loop_1++;
    }
}
int verificaQuantidadeComandos(char *tokens[]){
    int cont = 0, qtdComandos = 0;
    // .... | ..... | ..... | ..... => 3 pipes, porém 4 comandos,
    //por isso o qtdComandos++ no final
    while (tokens[cont] != NULL){
        if (strcmp(tokens[cont], STR_PIPE) == 0){
            qtdComandos++;
        }
        cont++;
    }
    qtdComandos++;
    return qtdComandos;
}
void trataTicket(char * tokens[], char* input, char* output, int opcoes){
    int descritor;
    pid = fork();
    if(pid == -1){
        showErroExecutarFork();
        return;
    }
    if(pid == 0){
        //Output
        if (opcoes == 0){
            //Abre o arquivo para ESCRITA
            descritor = open(output, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            //Realiza o dup
            dup2(descritor, STDOUT_FILENO);
            close(descritor);
        //Output e Input
        }else if (opcoes == 1){
            //Abre o arquivo
            descritor = open(input, O_RDONLY, 0600);
            //Realiza o dup
            dup2(descritor, STDIN_FILENO);
            close(descritor);
            // Same as before for the output file
            descritor = open(output, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(descritor, STDOUT_FILENO);
            close(descritor);
        }

        //Erro na execução
        if (execvp(tokens[0], tokens) == -1){
            showErroDefault();
            kill(getpid(),SIGTERM);
        }
    }
    //Espera
    waitpid(pid,NULL,0);
}
void launch(char** argumentos){
    pid = fork();
    if(pid == -1){
        showErroExecutarFork();
        return;
    }
    //Filho
    if(pid==0){
        signal(SIGINT, SIG_IGN);
        if (execvp(argumentos[0], argumentos) == -1){
            printf("Command not found");
            kill(getpid(),SIGTERM);
        }
    }
    waitpid(pid,NULL,0);
}

//ERROS
void showErro(char* toShow){
    printf("\n########################################################################################\n");
    printf("\nErro: %s\n", toShow);
    printf("\n########################################################################################\n");
}
void showErroExecutarFork(){
    showErro("Não foi possível realizar o fork!");
}
void showErroDefault(){
    showErro("Ops! Algo houve de errado!");
}

//SECRETS
void trataSecret(char* linha){
    if(strcmp(linha, "marco\n") == 0){
        printf("\n POLO! ( ^ ᗜ ^ )\n\n");
    } else if(strcmp(linha, "imsad\n") == 0){
        printf("\n Don't be sad! (っ˘з(˘⌣˘ )\n\n");
    } else if(strcmp(linha, "sus\n") == 0){
        printf("\n⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⣶⣿⣿⣷⣶⣄⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
               "⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⣾⣿⣿⡿⢿⣿⣿⣿⣿⣿⣿⣿⣷⣦⡀⠀⠀⠀⠀⠀\n"
               "⠀⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⡟⠁⣰⣿⣿⣿⡿⠿⠻⠿⣿⣿⣿⣿⣧⠀⠀⠀⠀\n"
               "⠀⠀⠀⠀⠀⠀⠀⣾⣿⣿⠏⠀⣴⣿⣿⣿⠉⠀⠀⠀⠀⠀⠈⢻⣿⣿⣇⠀⠀⠀\n"
               "⠀⠀⠀⠀⢀⣠⣼⣿⣿⡏⠀⢠⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿⣿⡀⠀⠀\n"
               "⠀⠀⠀⣰⣿⣿⣿⣿⣿⡇⠀⢸⣿⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⡇⠀⠀\n"
               "⠀⠀⢰⣿⣿⡿⣿⣿⣿⡇⠀⠘⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⢀⣸⣿⣿⣿⠁⠀⠀\n"
               "⠀⠀⣿⣿⣿⠁⣿⣿⣿⡇⠀⠀⠻⣿⣿⣿⣷⣶⣶⣶⣶⣶⣿⣿⣿⣿⠃⠀⠀⠀\n"
               "⠀⢰⣿⣿⡇⠀⣿⣿⣿⠀⠀⠀⠀⠈⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠀⠀⠀⠀\n"
               "⠀⢸⣿⣿⡇⠀⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠉⠛⠛⠛⠉⢉⣿⣿⠀⠀⠀⠀⠀⠀\n"
               "⠀⢸⣿⣿⣇⠀⣿⣿⣿⠀⠀⠀⠀⠀⢀⣤⣤⣤⡀⠀⠀⢸⣿⣿⣿⣷⣦⠀⠀⠀\n"
               "⠀⠀⢻⣿⣿⣶⣿⣿⣿⠀⠀⠀⠀⠀⠈⠻⣿⣿⣿⣦⡀⠀⠉⠉⠻⣿⣿⡇⠀⠀\n"
               "⠀⠀⠀⠛⠿⣿⣿⣿⣿⣷⣤⡀⠀⠀⠀⠀⠈⠹⣿⣿⣇⣀⠀⣠⣾⣿⣿⡇⠀⠀\n"
               "⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⣿⣦⣤⣤⣤⣤⣾⣿⣿⣿⣿⣿⣿⣿⣿⡟⠀⠀⠀\n"
               "⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⢿⣿⣿⣿⣿⣿⣿⠿⠋⠉⠛⠋⠉⠉⠁⠀⠀⠀⠀\n"
               "⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠉⠁⠀\n");
    }
}