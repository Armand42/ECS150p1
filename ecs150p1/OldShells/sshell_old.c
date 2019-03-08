#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
// CHANGE THE MAKEFILE
#define MAX 512
#define MAX_ARG 16

struct command {
	char buffer[MAX]; 
	int buffer_size;
	int _in; // T or F 
	int _out; // T or F
	char *outFile; // name of output file
	char *inFile;  // name of input file 
	int background;  // T or F 
	int num_single_cmd;
	single_command *commands_list[MAX]; // array of commands
} cmd;

struct single_command {
	char *args[MAX_ARG];
	int num_arg;
}

// ls -ll < infile | head -u | grep titi > outfile 
// ls -ll < infile    count: 0
// head -u 			  count: 1
// grep titi > outfile 	count: 2
void split_by_pipe() {
   const char p[2] = "|";
   char *token;
   token = strtok(cmd.buffer, p);
   int count = 0
   while( token != NULL ) {
      cmd.commands_list[count] = parsing_the_single_command(token);
      token = strtok(NULL, p);
   }
   cmd.num_single_cmd = count; 
}

void display_prompt() { // display shell prompt
	printf("sshell$ "); 
}

// ls -ll 
single_command parsing_the_single_command(char * token) {
	const char s[2] = " ";
    char *arg;
    arg = strtok(token, s);
    int count = 0
	single_command single_cmd; 
    while( arg != NULL ) {
       single_cmd.args[count] = arg;
       token = strtok(NULL, s);
    }
    single_cmd.num_arg = count;
    return single_cmd; 
}


void parsing_the_command2() {
    //  asfdsfds <      dfasfds
	// inFile: dfasfds
	// check for input redirection '<' 
	
	for(int i = 0; i < cmd.buffer_size; i++){
		if (cmd.buffer[i] == '<') {
			cmd.is_input_redirect = 1;		// check up until <
			int j = i+1;
			while (cmd.buffer[j] == ' ' && j < cmd.buffer_size) j++; // check after
			fprintf(stderr, "j = %d\n", j);
			if (j < cmd.buffer_size) {
				cmd.inFile = (char*)malloc(MAX*sizeof(char));
				int k = 0;
				while (cmd.buffer[j] != ' ' && j < cmd.buffer_size) {
					cmd.inFile[k++] = cmd.buffer[j++];
				}
				cmd.inFile[k] = '\0';
				cmd.buffer_size = i+1;		// make size smaller
			}
			// filename is empty
			fprintf(stderr, "File name: %s\n", cmd.inFile);
			break;
		} 
	}

	int i = 0;
	int count = 0;
	while (i < cmd.buffer_size) {					// loop through the buffer until reach max buffer_size
		cmd.args[count] = (char*)malloc(MAX*sizeof(char));  // allocate space of 512 per arg position (count)
		while (cmd.buffer[i] == ' ') i++; // skip any space per arg count
		if (i == cmd.buffer_size) break; // if all you have is space, then break
		int j = 0;
		//printf("before: i = %d\n", i);
		while (cmd.buffer[i] != ' ' && i < cmd.buffer_size){ // if it's not space and less than size, then copy into count position of arg
			cmd.args[count][j++] = cmd.buffer[i++]; // copy in arg when not space, i does not reset, goes all the way to buffer size
			//j++;
			//i++;
		}
		//printf("after: i = %d\n", i);
		cmd.args[count][j] = '\0'; // end string by placing \0 at end position
		count++;
	}
	cmd.num_arg = count;
}

void read_command() {
	for (int i = 0; i < MAX_ARG; i++)		// reset arg array each time after command
		cmd.args[i] = NULL;

	char pressEnter = '\n';
	memset(cmd.buffer, 0, MAX);		// reset entire array entries to 0
	fgets(cmd.buffer, MAX, stdin);		// copies all data from stdin up to 512 characters
	char ch;
	int i = 0;
	do {
		ch = cmd.buffer[i];
		i++;						// iterate through input string
	} while (ch != pressEnter);
	cmd.buffer[i-1] = '\0';  // special character in c which is saying the string is ending here 
	cmd.buffer_size = i-1;

	parsing_the_command();

	// printf("The command is: %s\n", cmd.buffer);
	// printf("The buffer size is: %d\n", cmd.buffer_size);
	// for(int i = 0; i < cmd.num_arg; i++){
	// 	printf("args[%d] = %s\n", i, cmd.args[i]);
	// }
}

int main(int argc, char *argv[])
{
	//char *cmd = "/bin/date -u";
	//char *cmd[3] = {"ls", "-l", NULL};		
	int status;
	int out;
	pid_t pid;

	// do some initialization
	cmd.is_input_redirect = 0;
	cmd.inFile = NULL;

	// char test[5] = "hello";
	// test[2] = '\0';
	// printf("Test: %s\n", test);

	//out = chdir("..");
	//printf("out = %d\n", out);
	
	while (1) {

		display_prompt();
		//printf("sshell$ ");
		read_command();

		pid = fork();			// forking off processes

		if (pid == 0) {			// if child process
			if (strcmp(cmd.args[0],"cd") == 0) {		// check if the command is cd
				if (chdir(cmd.args[1]) == 0){		// checks whether it succeeds or not
					//fprintf(stderr, "success: cd %s\n", cmd.args[1]);
					exit(EXIT_SUCCESS);		// exit with status 0, switch over to parent process
				}
				else{
					//fprintf(stderr, "failure: cd %s\n", cmd.args[1]);
					exit(EXIT_FAILURE);
				}
			}
			else if (strcmp(cmd.args[0], "exit") == 0) {
				exit(EXIT_SUCCESS);		// change status to 0, move to parent process
			}
			else if (strcmp(cmd.args[0],"pwd") == 0) {
				char* pwd = malloc(sizeof(char));	// allocate space for a string
				getcwd(pwd, MAX);	// return a string (path: Users/armandnasserischool/Desktop/ECS150/HW1) to pwd
				fprintf(stderr, "%s\n", pwd);
				exit(EXIT_SUCCESS);
			}
			else {
				out = execvp(cmd.args[0], cmd.args);		// execute the command in cmd
			}
			//fprintf(stderr, "Child exit: %d\n", out);
			exit(EXIT_FAILURE);
		}
		else if (pid > 0) {		// if parent process
			waitpid(-1, &status, 0);	// wait for child to be done (exit(EXIT_SUCCESS) -> status == 0)
			if (cmd.is_input_redirect == 1) {
				if (cmd.inFile == NULL) {
					fprintf(stderr, "Error: no input file\n");
				}
				else {
					int in_file = open(cmd.inFile, O_RDONLY);
					dup2(in_file, STDOUT_FILENO);
					close(in_file);
				}
			}
			if (status > 0) {
				if (strcmp(cmd.args[0],"cd") == 0) {
					fprintf(stderr, "Error: no such directory\n");
				}
				else {
					fprintf(stderr, "Error: command not found\n");
				}
			}
			else {
				if (strcmp(cmd.args[0], "cd") == 0){
					chdir(cmd.args[1]);
				}
				
				if (strcmp(cmd.args[0],"exit") == 0){
					fprintf(stderr, "Bye...\n");
					exit(EXIT_SUCCESS);		// exit in parent process exits entire program
				}
			}

			fprintf(stderr, "+ completed '%s' [%d]\n", cmd.buffer, WEXITSTATUS(status));		
		}
		else {
			fprintf(stderr, "failed to fork \n");
			exit(1);
		}
	}

    return EXIT_SUCCESS;
}
