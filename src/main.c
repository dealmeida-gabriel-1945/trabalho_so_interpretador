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

//CHAR
#define CHAR_LEFT_ARROW '<'
#define CHAR_RIGTH_ARROW '>'
#define CHAR_E_COMERCIAL '&'

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
//ERROS
void showErro(char* toShow);
void showErroExecutarFork();

// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;

static char* currentDirectory;
extern char** environ;

struct sigaction act_child;
struct sigaction act_int;

pid_t pid;


/**
 * SIGNAL HANDLERS
 */
// signal handler for SIGCHLD */
void signalHandler_child(int p);
// signal handler for SIGINT
void signalHandler_int(int p);


int changeDirectory(char * args[]);

/**
 * Method used to print the welcome screen of our shell
 */

/**
 * SIGNAL HANDLERS
 */

/**
 * signal handler for SIGCHLD
 */
void signalHandler_child(int p){
    /* Wait for all dead processes.
     * We use a non-blocking call (WNOHANG) to be sure this signal handler will not
     * block if a child was cleaned up in another part of the program. */
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
    printf("\n");
}

/**
 * Signal handler for SIGINT
 */
void signalHandler_int(int p){
    // We send a SIGTERM signal to the child process
    if (kill(pid,SIGTERM) == 0){
        printf("\nProcess %d received a SIGINT signal\n",pid);
    }else{
        printf("\n");
    }
}

/**
 *	Displays the prompt for the shell
 */
void shellPrompt(){
    // We print the prompt in the form "<user>@<host> <cwd> >"
    char hostn[1204] = "";
    gethostname(hostn, sizeof(hostn));
    printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}

/**
 * Method to change directory
 */
int changeDirectory(char* args[]){
    // If we write no path (only 'cd'), then go to the home directory
    if (args[1] == NULL) {
        chdir(getenv("HOME"));
        return 1;
    }
        // Else we change the directory to the one specified by the
        // argument, if possible
    else{
        if (chdir(args[1]) == -1) {
            printf(" %s: no such directory\n", args[1]);
            return -1;
        }
    }
    return 0;
}

/**
 * Method used to manage the environment variables with different
 * options
 */
int manageEnviron(char * args[], int option){
    char **env_aux;
    switch(option){
        // Case 'environ': we print the environment variables along with
        // their values
        case 0:
            for(env_aux = environ; *env_aux != 0; env_aux ++){
                printf("%s\n", *env_aux);
            }
            break;
            // Case 'setenv': we set an environment variable to a value
        case 1:
            if((args[1] == NULL) && args[2] == NULL){
                printf("%s","Not enought input arguments\n");
                return -1;
            }

            // We use different output for new and overwritten variables
            if(getenv(args[1]) != NULL){
                printf("%s", "The variable has been overwritten\n");
            }else{
                printf("%s", "The variable has been created\n");
            }

            // If we specify no value for the variable, we set it to ""
            if (args[2] == NULL){
                setenv(args[1], "", 1);
                // We set the variable to the given value
            }else{
                setenv(args[1], args[2], 1);
            }
            break;
            // Case 'unsetenv': we delete an environment variable
        case 2:
            if(args[1] == NULL){
                printf("%s","Not enought input arguments\n");
                return -1;
            }
            if(getenv(args[1]) != NULL){
                unsetenv(args[1]);
                printf("%s", "The variable has been erased\n");
            }else{
                printf("%s", "The variable does not exist\n");
            }
            break;


    }
    return 0;
}

/**
* Method for launching a program. It can be run in the background
* or in the foreground
*/
void launchProg(char **args, int background){
    int err = -1;

    if((pid=fork())==-1){
        printf("Child process could not be created\n");
        return;
    }
    // pid == 0 implies the following code is related to the child process
    if(pid==0){
        // We set the child to ignore SIGINT signals (we want the parent
        // process to handle them with signalHandler_int)
        signal(SIGINT, SIG_IGN);

        // We set parent=<pathname>/simple-c-shell as an environment variable
        // for the child
        setenv("parent",getcwd(currentDirectory, 1024),1);

        // If we launch non-existing commands we end the process
        if (execvp(args[0],args)==err){
            printf("Command not found");
            kill(getpid(),SIGTERM);
        }
    }

    // The following will be executed by the parent

    // If the process is not requested to be in background, we wait for
    // the child to finish.
    if (background == 0){
        waitpid(pid,NULL,0);
    }else{
        // In order to create a background process, the current process
        // should just skip the call to wait. The SIGCHILD handler
        // signalHandler_child will take care of the returning values
        // of the childs.
        printf("Process created with PID: %d\n",pid);
    }
}

/**
* Method used to manage I/O redirection
*/
void fileIO(char * args[], char* inputFile, char* outputFile, int option){

    int err = -1;

    int fileDescriptor; // between 0 and 19, describing the output or input file

    if((pid=fork())==-1){
        printf("Child process could not be created\n");
        return;
    }
    if(pid==0){
        // Option 0: output redirection
        if (option == 0){
            // We open (create) the file truncating it at 0, for write only
            fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            // We replace de standard output with the appropriate file
            dup2(fileDescriptor, STDOUT_FILENO);
            close(fileDescriptor);
            // Option 1: input and output redirection
        }else if (option == 1){
            // We open file for read only (it's STDIN)
            fileDescriptor = open(inputFile, O_RDONLY, 0600);
            // We replace de standard input with the appropriate file
            dup2(fileDescriptor, STDIN_FILENO);
            close(fileDescriptor);
            // Same as before for the output file
            fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fileDescriptor, STDOUT_FILENO);
            close(fileDescriptor);
        }

        setenv("parent",getcwd(currentDirectory, 1024),1);

        if (execvp(args[0],args)==err){
            printf("err");
            kill(getpid(),SIGTERM);
        }
    }
    waitpid(pid,NULL,0);
}

/**
* Method used to manage pipes.
*/
void pipeHandler(char * args[]){
    // File descriptors
    int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
    int filedes2[2];

    int num_cmds = 0;

    char *command[256];

    pid_t pid;

    int err = -1;
    int end = 0;

    // Variables used for the different loops
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;

    // First we calculate the number of commands (they are separated
    // by '|')
    while (args[l] != NULL){
        if (strcmp(args[l],"|") == 0){
            num_cmds++;
        }
        l++;
    }
    num_cmds++;

    // Main loop of this method. For each command between '|', the
    // pipes will be configured and standard input and/or output will
    // be replaced. Then it will be executed
    while (args[j] != NULL && end != 1){
        k = 0;
        // We use an auxiliary array of pointers to store the command
        // that will be executed on each iteration
        while (strcmp(args[j],"|") != 0){
            command[k] = args[j];
            j++;
            if (args[j] == NULL){
                // 'end' variable used to keep the program from entering
                // again in the loop when no more arguments are found
                end = 1;
                k++;
                break;
            }
            k++;
        }
        // Last position of the command will be NULL to indicate that
        // it is its end when we pass it to the exec function
        command[k] = NULL;
        j++;

        // Depending on whether we are in an iteration or another, we
        // will set different descriptors for the pipes inputs and
        // output. This way, a pipe will be shared between each two
        // iterations, enabling us to connect the inputs and outputs of
        // the two different commands.
        if (i % 2 != 0){
            pipe(filedes); // for odd i
        }else{
            pipe(filedes2); // for even i
        }

        pid=fork();

        if(pid==-1){
            if (i != num_cmds - 1){
                if (i % 2 != 0){
                    close(filedes[1]); // for odd i
                }else{
                    close(filedes2[1]); // for even i
                }
            }
            printf("Child process could not be created\n");
            return;
        }
        if(pid==0){
            // If we are in the first command
            if (i == 0){
                dup2(filedes2[1], STDOUT_FILENO);
            }
                // If we are in the last command, depending on whether it
                // is placed in an odd or even position, we will replace
                // the standard input for one pipe or another. The standard
                // output will be untouched because we want to see the
                // output in the terminal
            else if (i == num_cmds - 1){
                if (num_cmds % 2 != 0){ // for odd number of commands
                    dup2(filedes[0],STDIN_FILENO);
                }else{ // for even number of commands
                    dup2(filedes2[0],STDIN_FILENO);
                }
                // If we are in a command that is in the middle, we will
                // have to use two pipes, one for input and another for
                // output. The position is also important in order to choose
                // which file descriptor corresponds to each input/output
            }else{ // for odd i
                if (i % 2 != 0){
                    dup2(filedes2[0],STDIN_FILENO);
                    dup2(filedes[1],STDOUT_FILENO);
                }else{ // for even i
                    dup2(filedes[0],STDIN_FILENO);
                    dup2(filedes2[1],STDOUT_FILENO);
                }
            }

            if (execvp(command[0],command)==err){
                kill(getpid(),SIGTERM);
            }
        }

        // CLOSING DESCRIPTORS ON PARENT
        if (i == 0){
            close(filedes2[1]);
        }
        else if (i == num_cmds - 1){
            if (num_cmds % 2 != 0){
                close(filedes[0]);
            }else{
                close(filedes2[0]);
            }
        }else{
            if (i % 2 != 0){
                close(filedes2[0]);
                close(filedes[1]);
            }else{
                close(filedes[0]);
                close(filedes2[1]);
            }
        }

        waitpid(pid,NULL,0);

        i++;
    }
}

/**
* Main method of our shell
*/
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

        //Recolheo que o usuário digitar
        fgets(linha_comando, MAX_STR_LENGTH, stdin);

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
            fileIO(tokens_aux, tokens[i+1], tokens[i+3],1);
            return 1;

        //Se o comando for ">"
        }else if (strcmp(tokens[i], STR_RIGTH_ARROW) == 0){
            fileIO(tokens_aux, NULL, tokens[i+1], 0);
            return 1;
        }
        i++;
    }
    //Executa o resto dos tokens
    tokens_aux[i] = NULL;
    launchProg(tokens_aux,0);

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

//ERROS
void showErro(char* toShow){
    printf("\n########################################################################################\n");
    printf("\nErro: %s\n", toShow);
    printf("\n########################################################################################\n");
}
void showErroExecutarFork(){
    showErro("Não foi possível realizar o fork!");
}