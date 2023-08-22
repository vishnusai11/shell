//header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT 1024
#define MAX 100

int main(){
    pid_t pid1;
    char *split[MAX]; //to store the input after splitting the string at the |
    char *split1[MAX]; //to store the splitted input where the first element will be the command_name / program_name and the rest of the array are the arguments
    char *input1; 
    int i, num_cmds, num_cmds1, status, finalstatus;
    char *token;

    while(1){
        printf("jsh$ "); //printing the shell's prompt
        char input[MAX_INPUT] = {0}; //to store the user input
        //get command and put it in 'input' using fgets
        //if the input is 'ctrl+d' exits the shell
        if(!fgets(input, MAX_INPUT, stdin)) {  
            break; 
        }
        //when user presses 'enter' the shell prompt should be shown again for the user to enter input
        if(input[0] == '\n') {
            continue;
        }
        //remove the trailing newline
        input[strcspn(input, "\n")] = 0;
        input1 = &input;
        //split or seperate the input which may contain the | (pipe)
        for(num_cmds = 0; (token = strsep(&input1,"|")) != NULL; num_cmds++){ //the strsep seperates the input into seperate tokens
            //the token pointer returned by strsep put into a new array
            split[num_cmds] = token;
        }
        //Null termination
        split[num_cmds] = NULL;
        //create pipes
        int pipes[num_cmds-1][2]; //the pipes array is used to store the file descriptors for the pipes between the different child processes
        for(i = 0; i < num_cmds; i++) { 
            //Creates a pipe which consists of two file descriptors which correspond with the two "ends" of the pipe and these are stored in the pipes array
            if(pipe(pipes[i]) == -1) { 
                perror("pipe error");
                exit(EXIT_FAILURE);
            }
        }
        int pid[num_cmds]; //storing the pid of the child processes to be used by waitpid to check for the status
        char *command[num_cmds]; //storing the command_name / prog_name for the error message

        //iterating through each splitted_input in the order given by the user
        for(i = 0; i < num_cmds; i++) {
            //splitting the splitted_input into command_name / prog_name and arguments
            for(num_cmds1 = 0; (token = strsep(&split[i]," \t")) != NULL; num_cmds1++){
                //if there is a null character, avoiding that from the input which will be passed to execvp
                if(*token == '\0') {
                    num_cmds1--;
                    continue;
                }
                //the token pointer returned by strsep put into a new array
                split1[num_cmds1] = token;
            }
            //Null termination
            split1[num_cmds1] = NULL;

            //checking if the input is 'exit' -> the built-in command of this shell
            if (num_cmds1 == 1 && strcmp(split1[0], "exit") == 0) {
                return 0;
            }
            //starts forking ie., to create new process
            pid1 = fork(); 
            //check if there is an error while forking
            if(pid1 == -1){
                perror("fork error");
                exit(1); 
            }
            //check if we are in child process
            if(pid1 == 0){
                //output redirection
                //if it is not the first process, the input should be taken from the output of the previous process instead of taking from the keyboard input
                if(i > 0) {
                    dup2(pipes[i-1][0], STDIN_FILENO); 
                }
                //if it is not the last process, the output should be sent to the input of the next process instead of printing to the terminal
                if(i < num_cmds-1) {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }
                for(i=0; i<(num_cmds-1); i++){
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                //call execvp and if there is an error it exits with status 127 as given in the question
                if (execvp(split1[0], split1) == -1) {
                    exit(127);
                }
            } else {
                pid[i] = pid1;
                command[i] = split1[0];
            }
        }
        //parent process closes the file descriptors created by pipe()
        for(i=0; i<(num_cmds-1); i++){
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        //wait for the child processes to complete
        for(i = 0; i < num_cmds; i++) {
            waitpid(pid[i], &status, 0);
            status = WEXITSTATUS(status);
            //printing the error message 
            if(status == 127){
                printf("jsh error: Command not found: %s\n", command[i]);
            }
            //if it is the last process, printing it
            //this is so that, only the status of the last program in the pipeline is printed
            if(i == num_cmds - 1){
                finalstatus = status;
            }
        }
        printf("jsh status: %d\n", finalstatus);
    }//closing the while loop (the shell's)
    return 0;
}