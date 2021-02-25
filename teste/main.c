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

int checkFilePath(char *path);
void imprime(char* s);
char** getArgs(char** args, char* entrada, int *size);
char **getRedirect(char **args, int size, int* redirectSize);
int checkRedirect(char **args, int size);
char** getArgsSemRedirect(char** args, char** args_sem_redirect, int size, int*args_sem_redirect_size);

int main(int argc, char const *argv[]){

    char* entrada = (char*)malloc(1024*sizeof(char));
    printf("\nO que deseja mestre?\n");

    int cp[2];
    int pid;
    char ch;
    char *path = calloc(1024, sizeof(char));

    char **args;
    int status;
    int size = 0;
    char **redirect;
    char **args_sem_redirect;
    int args_sem_redirect_size;
    char *filepath = (char*)calloc( 1024, sizeof(char) );
    char *buffer = (char*)calloc( 1024, sizeof(char) );

    while(1){
        printf("\n$");

        fgets(entrada, 1024, stdin);

        if(entrada[strlen(entrada)-1] == '\n'){
            entrada[strlen(entrada)-1] = '\0';
        }

        if(!strcmp(entrada, "fim")){
            return 0;
        }
        if(!strcmp(entrada, "")){
            return 0;
        }

        if(!strcmp(entrada, "\n")){
            continue;
        }

        if( pipe(cp) < 0 ){
            perror("pipe nao pode ser feito");
            exit(1);
        }

        args = getArgs(args, entrada, &size);

        if( checkRedirect(args, size) ){
            printf("Simbolos como '<' e '>' não são validos. Favor tente novamente.\n");
            continue;
        }

        // for (int i = 0; i < size; ++i){
        // 	printf("%s\n", args[i]);
        // }

        sprintf(path, "/bin/%s", args[0]);
        int redirectSize = 0;
        if ( redirect = getRedirect(args, size, &redirectSize) ){
            // for (int i = 0; i < 2; ++i){
            // 	printf("%s\n", redirect[i]);
            // }

            args_sem_redirect = getArgsSemRedirect(args, args_sem_redirect, size, &args_sem_redirect_size);

            // for (int i = 0; i < 2; ++i){
            // 	printf("%s\n", args_sem_redirect[i]);
            // }


            for (int i = 0; i < redirectSize; ++i){
                if( !strcmp(redirect[i], "=>") ){

                    switch( pid = fork() ){
                        case -1:
                            perror("Can't fork");
                            exit(1);
                        case 0:
                            dup2(cp[1], 1);
                            close( cp[0]);
                            execvp(path, args_sem_redirect);
                            perror("No exec");
                            exit(1);

                        default:

                            waitpid(pid, &status, WCONTINUED);
                            close(cp[1]);
                    }


                    switch(pid = fork()) {
                        case -1:
                            perror("Can't fork");
                            exit(1);
                        case 0:
                            //define o caminho para o arquivo de saida
                            close(cp[1]);
                            sprintf(filepath,"/home/analele/Área de trabalho/SO_2019/%s", redirect[i+1]);

                            int filedesc = open(filepath, O_CREAT | O_WRONLY);

                            while(read(cp[0], &ch, 1)>0) {

                                write(filedesc, &ch, 1);
                            }

                            close( cp[0]);

                            exit(1);
                        default:
                            waitpid(pid, &status, WCONTINUED);

                    }
                }else if( !strcmp(redirect[i], "<=") ){

                    switch(pid = fork()) {
                        case -1:
                            perror("Can't fork");
                            exit(1);
                        case 0:

                            sprintf(filepath, "/home/analele/Área de trabalho/SO_2019/%s", redirect[i+1]);
                            if(!checkFilePath(filepath)){
                                printf("Arquivo não valido", filepath);
                            }

                            int filedesc = open(filepath, O_RDONLY);
                            dup2(filedesc,0);
                            dup2(cp[1],1);
                            while(read(filedesc, &ch, 1)){
                                write(cp[1], &ch, 1);
                            }
                            close(cp[0]);

                            exit(1);


                        default:

                            waitpid(pid, &status, WCONTINUED);
                            close(cp[1]);

                    }

                    switch(pid = fork()) {
                        case -1:
                            perror("Can't fork");
                            exit(1);
                        case 0:


                            dup2(cp[0],0);

                            read(cp[0], buffer, 1024);

                            args[args_sem_redirect_size+1] = buffer;
                            args[args_sem_redirect_size+2] = NULL;

                            for (int i = 0; i < args_sem_redirect_size; ++i){
                                printf("%s\n", args[i]);
                            }


                            execvp(path, args);
                            perror("No exec");

                            exit(1);


                        default:

                            waitpid(pid, &status, WCONTINUED);
                            close(cp[0]);

                    }
                }
            }




        }else{

            switch( pid = fork()){

                case -1:
                    perror("fork nao pode ser feito");
                    exit(1);
                case 0:

                    close(cp[0]);
                    dup2(cp[1], 1);
                    execvp(path, args);
                    perror("nao pode executar");
                    exit(1);
                default:
                    close(cp[1]);

                    waitpid(pid, NULL, WCONTINUED);

                    while(read(cp[0], &ch, 1) > 0){
                        write(0, &ch, 1);
                    }

            }
        }




    }

    return 0;
}

int checkFilePath(char *path){
    FILE *f;
    if(fopen(path, "r")){
        return 1;
    }else{
        return 0;
    }
}

char** getArgsSemRedirect(char** args, char** args_sem_redirect, int size, int*args_sem_redirect_size){

    for (int i = 0; i < size; ++i){
        if( !strcmp(args[i], "=>") || !strcmp(args[i], "<=") ){
            *args_sem_redirect_size = i;
        }
    }

    args_sem_redirect = (char**)malloc( *args_sem_redirect_size * sizeof(char*) );
    for (int i = 0; i < *args_sem_redirect_size; ++i){
        args_sem_redirect[i] = (char*)malloc( 1024 * sizeof(char) );
    }

    for (int i = 0; i < *args_sem_redirect_size; ++i){
        strcpy(args_sem_redirect[i], args[i]);
    }

    return args_sem_redirect;
}

int checkRedirect(char **args, int size){

    for (int i = 0; i < size; ++i){
        if( !strcmp(args[i], ">") || !strcmp(args[i], "<") ){

            return 1;
        }
    }
    return 0;

}

char **getRedirect(char **args, int size, int* redirectSize){

    char** redirect;
    for (int i = 0; i < size; ++i){
        if( !strcmp(args[i], "=>") || !strcmp(args[i], "<=") ){
            *redirectSize = *redirectSize + 1;
        }
    }
    if(*redirectSize > 0){
        *redirectSize = 2 * (*redirectSize);
        int j = 0;
        redirect = (char**)malloc((*redirectSize) * sizeof(char*) );
        for (int i = 0; i < (*redirectSize); ++i){
            redirect[i] = (char*)malloc( 1024 * sizeof(char) );
        }
        for (int i = 0; i < size; ++i){
            if( !strcmp(args[i], "=>") || !strcmp(args[i], "<=") ){


                strcpy(redirect[j], args[i]);
                j++;
                strcpy(redirect[j], args[i+1]);
                j++;
            }
        }

        return redirect;
    }else{
        return NULL;
    }
}

char** getArgs(char** args, char* entrada, int* size){

    char *copia = (char*)malloc(strlen(entrada)* sizeof(char));
    strcpy(copia, entrada);

    char *token;

    /* get the first token */
    token = strtok(copia, " ");

    /* walk through other tokens */
    *size = 0;
    while( token != NULL ) {
        token = strtok(NULL, " ");
        *size = *size + 1;
    }
    args = (char**)malloc((*size) * sizeof(char*));
    for(int j = 0; j < *size; j++){
        args[j] = (char*)malloc(1024 * sizeof(char));
    }
    token = strtok(entrada, " ");
    int i = 0;
    while( token != NULL ) {
        strcpy(args[i], token);
        token = strtok(NULL, " ");
        i++;
    }

    return args;

}

void imprime(char* s){
    printf("%s\n", s);
}