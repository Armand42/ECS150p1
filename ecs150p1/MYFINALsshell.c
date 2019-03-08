#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
// Macros used for buffers
#define MAX 512
#define MAX_ARG 16
#define IN 0
#define OUT 1

// Command data structure specific to cd, pwd, exit
struct single_command {
    char *args[MAX_ARG];    // max args of 16
    int num_arg;            // number of arguments counter
};
// General command data structure
struct command {
    char ori_buffer[MAX];
    char buffer[MAX];
    int buffer_size;
    int _in; // T or F
    int _out; // T or F
    char *outFile; // name of output file
    char *inFile;  // name of input file
    int background;  // T or F
    int num_cmd;
    char *cmd_array[MAX];
    struct single_command list[MAX]; // array of commands
} cmd;

// Takes a string and removes all whitespace    REFERENCE CODE
char * trim_space(char *str) {
    if (str == NULL) return NULL;
    char *end;
    while (isspace(*str)) str++;
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end+1) = '\0';
    return str;
}
// Split by whitespace and store each command in struct single_command
struct single_command parsing_the_single_command(char * str) {
	char *arg;
    arg = strtok(str, " ");
    int count = 0;
	  struct single_command ret;
    while( arg != NULL ) {
        ret.args[count++] = arg;        
        arg = strtok(NULL, " ");
    }
    ret.num_arg = count;
    return ret;
}

struct single_command parsing_the_redirection(char * str, int dir) {
    char * s = dir == OUT ? ">" : "<";
    char *arg[2];
    char *token;
    token = strtok(str, s);
    arg[0] = trim_space(token);
    token = strtok(NULL, s);
    arg[1] = trim_space(token);
    struct single_command ret;
    ret = parsing_the_single_command(arg[0]);
    if (arg[1] != NULL) {
        if (dir == OUT) {
            cmd._out = 1;
            cmd.outFile = arg[1];
        }
        else {
            cmd._in = 1;
            cmd.inFile = arg[1];
        }
    }
    if (cmd._in == 1 && cmd.inFile == NULL) {
        fprintf(stderr, "Error: no input file\n");
        exit(1);
    }
    if (cmd._out == 1 && cmd.outFile == NULL) {
        fprintf(stderr, "Error: no output file\n");
        exit(1);
    }
    return ret;
} 

void split_by_pipe() {
    char *token;
    token = strtok(cmd.buffer, "|");
    int count = 0;
    while( token != NULL ) {
        cmd.cmd_array[count++] = trim_space(token);
        token = strtok(NULL, "|");
    }
   
    cmd.num_cmd = count;
    for(int i = 0; i < cmd.num_cmd; i++){
        if (i == 0) {
            cmd.list[i] = parsing_the_redirection(cmd.cmd_array[i], IN);
        }
        else if (i == cmd.num_cmd - 1) {
            cmd.list[i] = parsing_the_redirection(cmd.cmd_array[i], OUT);
        }
        else {
            cmd.list[i] = parsing_the_single_command(cmd.cmd_array[i]);
        }
    }
}

void print_command() {
    printf("-------------------------------------------\n");
    printf("There are %d simple commands\n", cmd.num_cmd);
    for(int i = 0; i < cmd.num_cmd; i++){
        printf("  Command %d: %d args\n", i+1, cmd.list[i].num_arg);
        for(int j = 0; j < cmd.list[i].num_arg; j++)
            printf("    args[%d] = '%s'\n", j, cmd.list[i].args[j]);
	  }
    printf("Infile : %d   Filename: '%s'\n", cmd._in, cmd.inFile);
    printf("Outfile: %d   Filename: '%s'\n", cmd._out, cmd.outFile);
    printf("Background: %d\n", cmd.background);
    printf("-------------------------------------------\n");
}



void read_command() {
	fgets(cmd.buffer, MAX, stdin);
    int n = strlen(cmd.buffer);
    cmd.buffer[n-1] = '\0'; //remove new line character
    strcpy(cmd.ori_buffer, cmd.buffer);
    trim_space(cmd.buffer);
    n = strlen(cmd.buffer);
    
    if (cmd.buffer[n-1] == '&') {
        cmd.background = 1;
        cmd.buffer[n-1] = '\0';
        trim_space(cmd.buffer);
    }
	  split_by_pipe();
}
// Creates process for single command
void create_process(struct single_command scmd) {
    if (strcmp(scmd.args[0],"cd") == 0) {		// check if the command is cd (0 means yes)
        if (chdir(scmd.args[1]) != 0)       // if it can't change directories
             fprintf(stderr, "Error: no such directory\n");
        return;
    }
    if (strcmp(scmd.args[0], "exit") == 0) {        
        fprintf(stderr, "Bye...\n");
        exit(0);
    }
    if (strcmp(scmd.args[0],"pwd") == 0) {
        char* pwd = malloc(sizeof(char));	// allocate space for a string
        getcwd(pwd, MAX);	// return a string (path: Users/armandnasserischool/Desktop/ECS150/HW1) to pwd
        printf("%s\n", pwd);
        return;
    }
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        execvp(scmd.args[0], scmd.args);    // specific command, list of arguments
    }
}

void execute() {
    int savedin, savedout, fdin, fdout, status;
    int fdpipe[2];

    savedin = dup(0); // stdin
    savedout = dup(1); // stdout

    fdin = cmd._in ? open(cmd.inFile, O_RDONLY) : dup(savedin);
    // cat < infile | grep tiki
    //  fdin : content of the file
    //  fdout:  result of the command "cat < file"
    // go to write end of the pipe 
    // go to through the pipe
    // go to the read end of the pipe: fdpipe[0]
    // set fdin = read end of the pipe
    // there are n commands, we did n-1 in the loop
    
    //  (write end fdpipe[1]) -> PIPE -> (read end fdpipe[0])  -> PIPE -> 
    //   fdout
    for (int i = 0; i < cmd.num_cmd-1; i++) { // each iteration takes the input of the previous, outputs to the next input of next command
        dup2(fdin, 0);
        close(fdin);
        pipe(fdpipe);
        fdin = fdpipe[0];   // uses command as next input the STDOUT of pipe
        fdout = fdpipe[1];  // point to write end of pipe
        dup2(fdout, 1);
        close(fdout);
        // running the cmd.list[i]: input (fdin), output(fdout)
        create_process(cmd.list[i]); // first input either file or STDIN
        // runs command, sends through pipe 
    }
    // Every iteration you have to fork, but exevcp prevents exponential growth
    // cat < inFile | sort | unique | grep tiki > outFile 
    //   cmd1 (child process1)  | cmd2 (child process2)  | cmd3 (child process3) | cmd4 (child process 4) 
    // parent process: fork to a new process to run the cmd 
    // input for cmd1: inFile, stdin  (process1)
    // output for cmd1  -> as the input to cmd2  (pipe)
    // output for cmd2  -> as the input to cmd3  (pipe)
    // output for cmd3  -> as the input to cmd4 
    // output for cmd4  -> outFile or stdout 
                        // either file or STDOUT for final cmd 
    fdout = cmd._out ? open(cmd.outFile, O_WRONLY | O_CREAT, 0644) : dup(savedout);
    dup2(fdin, 0);
    close(fdin);
    dup2(fdout, 1);
    close(fdout);
    create_process(cmd.list[cmd.num_cmd-1]);

    dup2(savedin, 0);
    dup2(savedout, 1);
    close(savedin);
    close(savedout);

    if (!cmd.background) {
      waitpid(-1, &status, 0);
      // if (status > 0)
      //     fprintf(stderr, "Error: command not found\n");
      fprintf(stderr, "+ completed '%s' [%d]\n", cmd.ori_buffer, WEXITSTATUS(status));
    }
}
// Reset all buffers and variables to zero after execution
void reset() {
    memset(cmd.list, 0, sizeof(cmd.list));
    cmd.num_cmd = 0;
    memset(cmd.buffer, 0, MAX);
    cmd._in = 0; // T or F
    cmd._out = 0; // T or F
    cmd.outFile = NULL; // name of output file
    cmd.inFile = NULL;  // name of input file
    cmd.background = 0;  // T or F
}

int main(int argc, char *argv[])
{
	  while (1) {
        printf("sshell$ ");
		read_command();
        //print_command();
        execute();
        reset();
	  }

    return 0;
}
