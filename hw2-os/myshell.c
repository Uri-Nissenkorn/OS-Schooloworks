#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


int pipe_command(char **arglist, int command_id) {
    char ** command = arglist + command_id;
    int p[2];
    if (pipe(p) == -1){
        printf("Pipe has failed : %s\n", strerror(errno));
        return 0;
    }
    pid_t pid = fork();
    if (pid == -1) {
        printf("Fork has failed: %s\n", strerror(errno));
        return 0;
    }
    else if (pid == 0){ // first command
        close(p[0]);
        if (dup2(p[1], STDOUT_FILENO) == -1) {
			printf("dup2 failed, Error: %s\n", strerror(errno));
			exit(1);
		}
        close(p[1]);
        execvp(arglist[0], arglist);
        exit(1);
    }
    else{
        close(p[1]);
        pid_t pid2 = fork();
        if (pid2 == -1) {
            printf("Fork has failed: %s\n", strerror(errno));
            return 0;
        }
        else if (pid2 == 0){ // second command
            if (dup2(p[0], STDIN_FILENO) == -1) {
                printf("dup2 failed, Error: %s\n", strerror(errno));
			    exit(1);
            }
            close(p[0]);
            execvp(command[0], command);
            exit(1);
        } else {
            close(p[0]);
            waitpid(pid, NULL, WUNTRACED); 
            waitpid(pid2, NULL, WUNTRACED); 
        }
    }
    return 0;
}

//===============================================


int prepare(void) {
    return 0;
}

int process_arglist(int count, char **arglist) {
    int piping = 0;

    // Output redirection:
    if (count > 2 && strcmp(arglist[count-2],">>")==0) {

        int file = open(arglist[count-1],O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (file == -1) {
            printf("Error: %s\n", strerror(errno));
            return 1;
        }
        arglist[count-1] = NULL;
        arglist[count-2] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
                dup2(file, 1) ; 
                execvp(arglist[0], arglist);
                exit(1);
            }
        else{
            waitpid(pid, NULL, WUNTRACED); 
        }

    // Executing commands in the background:
    } else if (count > 1 && strcmp(arglist[count-1],"&")==0) {
        if (fork() == 0) {
            arglist[count-1] = NULL;
            execvp(arglist[0], arglist);
            exit(1);
        }

    } else {
        for (int i = 0; i < count; i++){
            if (strcmp(arglist[i],"|")==0) {
                piping = i;
                arglist[i]=NULL;
            }
        }

        // Piping:
        if(piping!=0) {
           pipe_command(arglist, piping+1);

        // Default:
        } else {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(arglist[0], arglist);
                exit(1);
            }
            else{
                waitpid(pid, NULL, WUNTRACED); 
            }
        }
    }
    return 1;
}

int finalize(void) {
    return 0;
}



