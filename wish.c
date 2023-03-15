#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void em()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void copyarr(char *dest[], char *src[])
{
    for (int i = 0; src[i] != NULL; i++)
    {
        dest[i] = strdup(src[i]); 
    }
}


int main(int argc, char *argv[])
{
    FILE *f = NULL; 
    size_t bufsize = 512;
    char *buffer = NULL;
    char *ogpath = strdup("/bin/");

    if(argc == 1) f = stdin;  // interactive mode
    else if (argc == 2) //  arg batch file 
    {
        f =  fopen(argv[1], "r");
        if (f == NULL) {em(); exit(1);};
    }
    else {em(); exit(1);}// more than one argument throw an error msg. 

    while(1){
        if(argc == 1) printf("wish>");
        int tlen = 0;
        char *t, *token[10], *paths[10];
        memset(token, 0, sizeof(token));
        
        int ch = getline(&buffer, &bufsize, f); 

        if (ch == EOF) exit(0); 
        if (buffer[ch-1] == '\n') buffer[ch-1] = '\0';
        for (; *buffer && strchr(" \t\r\n\v", *buffer); buffer++);
        if(*buffer == '\0') continue;

        while(t = strsep(&buffer, " ")) //seperating input by space
        {
            for (; *t && strchr("\t\r\n\v", *t); t++);
            if(*t == '\0') continue;
            token[tlen]=t; 
            tlen++;
        }
        
        if (strcmp(token[0], "cd") == 0) 
        {
            if(tlen != 2) {em(); break;} // check # of args 
            if(chdir(token[1])==-1) em(); 
        }

        else if (strcmp(token[0], "path") == 0) 
        {
            memset(paths, 0, sizeof(paths));
            if(token[1] == NULL) ogpath = NULL; 
            for (int i = 1; token[i] != NULL; i++) 
            {
                paths[i-1] = strcat(token[i],"/");
            }
        } 
        
        else if (strcmp(token[0], "if") == 0 && strcmp(token[tlen-1], "fi") == 0) // if statement TODO
        {
            if(ogpath == NULL) {em(); exit(0);}
            
            char *command1[10], *command2[10], *ttoken[10]; 
        
            copyarr(ttoken, token); 

            int fic = 0, sh = 1, ifc = 0;
            int rdc = 0, indi=-1;
            char *fn1;
            for (int i = 0; token[i] != NULL; i++)
            {
                if(strcmp(token[i], "fi") == 0) fic++; 
                if(strcmp(token[i], "if") == 0) ifc++;
            }
            if(fic != ifc) {em(); exit(0);}

            int opind[fic+1], op[fic+1], then[fic+2], iif[fic+1];
            then[0] = 0;
            iif[0] = 0;

            for (int fi = 0; fi<fic; fi++)
            {
                opind[fi] = -1;
                op[fi] = -1;
                then[fi+1] = -1;
                for (int i = then[fi]+1; i<tlen-1; i++)
                {
                    if (strcmp(token[i], "if") == 0) {iif[fi] = i;}
                    if (strcmp(token[i], "==") == 0) {opind[fi] = i; op[fi] = 1;}
                    if (strcmp(token[i], "!=") == 0) {opind[fi] = i; op[fi] = -1;}
                    if (strcmp(token[i], "then") == 0) {then[fi+1] = i; break;}
                }
                if (opind[fi] == -1 || then[fi+1] == -1) {em(); exit(0);}
                if (opind[fi] == 1 || opind[fi]+3 == tlen -1) exit(0);
            }

            for (int fi = 0; fi<fic; fi++)
            {
                memset(command1, 0, sizeof(command1));
                int x1, x2 = atoi(ttoken[opind[fi]+1]);
                for (int i = iif[fi]+1; i<opind[fi]; i++)
                {
                    command1[(i-(iif[fi]+1))] = strdup(ttoken[i]);
                }
                
                pid_t pid3 = fork();  

                if (pid3 < 0) exit(0); 
                else if (pid3 == 0)
                {
                    char *tpath = strdup(ogpath);
                    if(access(strcat(tpath, command1[0]), X_OK) == 0) // system call
                    {
                        if(execv(command1[0], command1) == -1) exit(0);
                        exit(0);
                    }
                    else //not system call
                    {
                        command1[0] = strdup(strcat(strdup("./"),command1[0]));
                        if(execv(command1[0], command1) == -1) exit(0);
                    }
                    exit(0);
                }
                else 
                {
                    int status; 
                    waitpid(pid3, &status, 0); 
                    x1 = WEXITSTATUS(status); 
                    if (!((op[fi] == 1 && x1 == x2) || (op[fi] == -1 && x1 != x2))) sh = -1;
                }

            }
            
            for (int i = then[fic]+1; strcmp(token[i], "fi") != 0; i++)
            {
                command2[i - (then[fic]+1)] = strdup(ttoken[i]);
            }
            
            for (int i = 0; command2[i] != NULL; i++) 
            {
                if (strcmp(command2[i], ">") == 0) {rdc++; indi = i;} 
                if (rdc > 1) exit(0); 
            }


            if (sh == 1) // TODO
            {
                if(strcmp(command2[0],"cd") == 0) 
                {
                    chdir(command2[1]); 
                }  
                else if (rdc == 1)
                { 
                    fn1 = strdup(command2[indi+1]);
                    char *tpath;
                    for (int i = indi; command2[i] != NULL; i++) command2[i] = NULL;
                    
                    pid_t pid = fork(); 
                    if (pid < 0) {em(); exit(0);}
                    else if (pid == 0) // child processor
                    {
                        int fd = open(fn1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        tpath = strdup(ogpath);
                        dup2(fd, STDOUT_FILENO);
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                        strcat(tpath,command2[0]);
                        if(access(tpath, X_OK) == 0) 
                        {
                            if(execv(tpath, command2) == -1) {exit(0);}
                        }
                        command2[0] = strdup(strcat(strdup("./"),command2[0])); 
                        execv(command2[0], command2);
                        exit(0);
                    }
                    else // parent  
                    {
                        int status = 0; 
                        waitpid(pid, &status, 0); //TODO 
                    }
                }
                else 
                {
                    char *tpaths[10], *tpath; 
                    pid_t pid1 = fork();
                    if (pid1 < 0) exit(0); 
                    else if (pid1 == 0)
                    {
                        if (paths[0] != NULL)
                        {
                            copyarr(tpaths, paths);
                            for (int i = 0; paths[i] != NULL; i++) // path existing
                            { 
                                char *new_command = strcat(tpaths[i],command2[0]);
                                if(access(new_command, X_OK) == 0) 
                                    execv(new_command, command2); 
                            }
                        }
                        
                        tpath = strdup(ogpath);
                        char *new_command = strcat(tpath,command2[0]);            
                        if(access(new_command, X_OK) == 0) 
                        {
                            if(execv(new_command, command2) == -1) {exit(1);}
                        }

                        command2[0] = strdup(strcat(strdup("./"),command2[0])); 
                        execv(command2[0], command2);
                        exit(0);
                    }
                    
                    else 
                    {
                        int status;
                        waitpid(pid1, &status, 0); 
                    }

                }
            }
        }
        

        else if (strcmp(token[0], "exit") == 0) 
        {
            if (token[1] != NULL) em();
            exit(0);
        }
        
        else // for not built-in commands 
        {   
            int rdc = 0, indi=-1, indj=-1, fni = 0;
            char fn1[128];
            
            //check Redirection
            for (int i = 0; token[i] != NULL; i++) 
            {
                for (int j = 0; j < strlen(token[i]); j++)
                {   
                    if(rdc>1) {em(); exit(0);} // multiple > signs
                    if(token[indi+1] != NULL && indi!=-1) {em(); exit(0);} // multiple w/o space

                    if(token[i][j] == '>')
                    {
                        rdc++;
                        if(j==0) // space
                        {
                            if(token[i+1] == NULL || i == 0) {em(); exit(0);} 
                            indi = i+1;
                        }
                        if(j>0) // w/o space
                        {
                            if(token[i][j+1] == '\0' || i == 0) {em(); exit(0);}
                            indi=i;
                        }
                        indj = j; 
                    }
                    if(rdc == 1 && indj>0 && j>indj) {fn1[fni]= (token[i][j]); fni++;} // > w/o space
                    if(rdc == 1 && indj==0) {strcpy(fn1,token[indi]);} // > w/ space
                }
            }

            if (rdc == 1) // if redirection
            {   
                char *nt[10], *tpath;
                if (indj==0) // space
                {
                    copyarr(nt,token); 
                    for (int i = indi-1; nt[i+1] != NULL; i++)
                    {
                        nt[i] = NULL;
                    }
                }
                if (indj>0) // w/o space
                {
                    for (int i = 0; token[i] != NULL; i++)
                    {
                        if(i==indi) nt[i] = strndup(token[i], indj);
                        else nt[i] = strdup(token[i]);
                    }
                }
                
                pid_t pid = fork(); 
                if (pid < 0) {em(); exit(0);}
                else if (pid == 0) // child processor
                {
                    int fd = open(fn1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    tpath = strdup(ogpath);
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                    strcat(tpath,nt[0]);
                    if(access(tpath, X_OK) == 0) 
                    {
                        if(execv(tpath, nt) == -1) {exit(0);}
                    }
                    else {em();}
                    exit(0);
                }
                else // parent  
                {
                    int status = 0; 
                    waitpid(pid, &status, 0); //TODO 
                }
            }
            
            else // no redirection
            {   
                char *tpaths[10], *tpath;
                pid_t pid = fork();
                if (pid < 0) exit(0);
                else if (pid == 0)
                {
                    if(ogpath == NULL) {em(); exit(0);} 
                    else
                    {      
                        if(paths[0] != NULL){
                            copyarr(tpaths, paths);
                            for (int i = 0; paths[i] != NULL; i++) // path existing
                            {
                                char *new_command = strcat(tpaths[i],token[0]);
                                if(access(new_command, X_OK) == 0) execv(new_command, token); 
                            }
                        }
                        tpath = strdup(ogpath);
                        char *new_command = strcat(tpath,token[0]);  
                        if(access(new_command, X_OK) == 0) 
                        {
                            if(execv(new_command, token) == -1) {exit(1);}
                        }
                        else {em(); exit(1);}
                    }
                    exit(0);  
                    
                } 
                else
                {
                    int status = 0; 
                    waitpid(pid, &status, 0); 
                    if(WEXITSTATUS(status) == 1) exit(0);
                }   
            }
        }         
    }
}
