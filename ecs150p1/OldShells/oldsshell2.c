#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX 512
#define MAX_ARG 16
#define IN 0
#define OUT 1
// string s = "ls -l"    execvp(scmd.args[0], scmd.args)
//  single_command1= {["ls", "-l"], 2}, 
// single_command2 = {["sort", "-n"], 2},
// single_command3 = {["grep", "titi"], 2},
// list = [single_command1, single_commnad2, single_command3]
// cmd_array = ["ls -l > infle", "sort -n", "grep titi < outfiel"];
struct single_command {
    char *args[MAX_ARG];
    int num_arg;
};
//     ls    -l >    infile     |    | sort -n | grep titi < outfile
struct command {
    char ori_buffer[MAX];
    char buffer[MAX];
    int buffer_size;
    int _in; // T or F
    int _out; // T or F
    char *outFile; // name of output file
    char *inFile;  // name of input file
    int background;  // T or F
    int num_cmd;     // number of single commands 
    char *cmd_array[MAX]; // cmd_array[0] = "ls -l > infile", cmd_array[1] = "sort -n", cmd_array[2] = "grep titi < outfile"  // AFTER YOU SPLIT BY '|'
    struct single_command list[MAX]; // array of single commands
	// list is array of size 3
	// list[0] = single_command 1 (2 args: 'ls' '-l'), execvp(cmd.list[0].args[0], cmd.list[0].args)
	// list[1].args[0] 
	// list[2] 
} cmd;

//"    ls    "  --> "ls"
// remove leading spaces and trailing space
// isspace(c): T if c is ' ' '\t' '\n' 
char * trim_space(char *str) {
    if (str == NULL) return NULL;
    char *end;
    while (isspace(*str)) str++;
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end+1) = '\0';
    return str;
}

//"ls   -l   -h   -a"
// {["ls", "-l"], 2}

// "ls" arg[0]   count = 0
// "-l" arg[1]	 count = 1
//		num_arg =		 count = 2
struct single_command parsing_the_single_command(char * str) {
	char *arg;
    arg = strtok(str, " ");
    int count = 0;
	struct single_command ret;
    while( arg != NULL ) {
        ret.args[count++] = arg;			// store current arg into args[i]
        arg = strtok(NULL, " ");       //"ls      -l   -h   -a"
    }
    ret.num_arg = count;
    return ret;				// return object of single command
}

// "ls -l < infile", IN or OUT
struct single_command parsing_the_redirection(char * str, int dir) {
    char * s = dir == OUT ? ">" : "<";
    char *arg[2];
    char *token;
    token = strtok(str, s);			// split by direction < or >
    arg[0] = trim_space(token);			// "   ls       -l  <   infile   " 
    token = strtok(NULL, s);      // arg[0] = "ls       -l"
    arg[1] = trim_space(token);   // arg[1] = "   infile    "
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
    return ret;
}
// ls    -l >    infile | sort -n | grep titi < outfile

void split_by_pipe() {
    char *token;
    token = strtok(cmd.buffer, "|");
    int count = 0;
    while( token != NULL ) {
        cmd.cmd_array[count++] = trim_space(token);
        token = strtok(NULL, "|");
    }
    cmd.num_cmd = count;
	//   a  >  infile | b | c | d < outfile 
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
	fgets(cmd.buffer, MAX, stdin);		// read in command up to 512 chars
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

void execute() {
    int tmpin = dup(0);
    int tmpout = dup(1);
    int fdin;
    if (cmd._in) {
        fdin = open(cmd.inFile, O_RDONLY);
    }
    else {
        fdin = dup(tmpin);
    }
    int pid;
    int status;
    int fdout;
	for(int i = 0; i < cmd.num_cmd; i++) {
        dup2(fdin, 0);
        close(fdin);
        if (i == cmd.num_cmd - 1) {
            if (cmd._out) {
                fdout = open(cmd.outFile, O_WRONLY | O_CREAT, 0644);
            }
            else {
                fdout = dup(tmpout);
            }
        }
        else {
            int fdpipe[2];
            pipe(fdpipe);
            fdout = fdpipe[1];
            fdin = fdpipe[0];
        }

        dup2(fdout, 1);
        close(fdout);
        pid = fork();

		if (pid == 0) {	// list [ls, -l,] args[0] = ls
					// command				list of commands
			execvp(cmd.list[i].args[0], cmd.list[i].args);
			exit(EXIT_FAILURE);
		}
    }

    dup2(tmpin, 0);
    dup2(tmpout, 1);
    close(tmpin);
    close(tmpout);

    // if (!cmd.background) {
    //     waitpid(pid, NULL);
    // }
    waitpid(-1, &status, 0);
    fprintf(stderr, "+ completed '%s' [%d]\n", cmd.ori_buffer, WEXITSTATUS(status));

}
// void execute() {
//   int pid = fork();			// forking off processes
//
//   if (pid == 0) {			// if child process
//       if (strcmp(cmd.args[0],"cd") == 0) {		// check if the command is cd
//           if (chdir(cmd.args[1]) == 0)
//               exit(EXIT_SUCCESS);
//           else
//               exit(EXIT_FAILURE);
//       }
//       else if (strcmp(cmd.args[0], "exit") == 0) {
//           exit(EXIT_SUCCESS);		// change status to 0, move to parent process
//       }
//       else if (strcmp(cmd.args[0],"pwd") == 0) {
//           char* pwd = malloc(sizeof(char));	// allocate space for a string
//           getcwd(pwd, MAX);	// return a string (path: Users/armandnasserischool/Desktop/ECS150/HW1) to pwd
//           fprintf(stderr, "%s\n", pwd);
//           exit(EXIT_SUCCESS);
//       }
//       else {
//           out = execvp(cmd.args[0], cmd.args);		// execute the command in cmd
//       }
//       exit(EXIT_FAILURE);
//   }
//   else if (pid > 0) {		// if parent process
//       waitpid(-1, &status, 0);	// wait for child to be done (exit(EXIT_SUCCESS) -> status == 0)
//       if (cmd._in == 1) {
//           if (cmd.inFile == NULL) {
//             fprintf(stderr, "Error: no input file\n");
//           }
//           else {
//             int in_file = open(cmd.inFile, O_RDONLY);
//             dup2(in_file, STDOUT_FILENO);
//             close(in_file);
//           }
//       }
//       if (status > 0) {
//         if (strcmp(cmd.args[0],"cd") == 0) {
//           fprintf(stderr, "Error: no such directory\n");
//         }
//         else {
//           fprintf(stderr, "Error: command not found\n");
//         }
//       }
//       else {
//         if (strcmp(cmd.args[0], "cd") == 0){
//           chdir(cmd.args[1]);
//         }
//
//         if (strcmp(cmd.args[0],"exit") == 0){
//           fprintf(stderr, "Bye...\n");
//           exit(EXIT_SUCCESS);
//         }
//       }
//
//       fprintf(stderr, "+ completed '%s' [%d]\n", cmd.buffer, WEXITSTATUS(status));
//     }
//     else {
//       fprintf(stderr, "failed to fork \n");
//       exit(1);
//     }
// }

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
	  int status;
	  int out;
	  pid_t pid;

	  while (1) {
        printf("sshell$ ");
		read_command();
        //print_command();
        execute();
        reset();
	  }

    return 0;
}
