#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#define MAX 512
#define MAX_ARG 16
// Issues: Need to trim by tab, check for &,<,>,| for errors, 
// Fixes: Fixed command printing by copying original input
struct command {
char orig_buffer[MAX];			// buffer: first thing read into program, we parse and do stuff
char new_buffer[MAX];			// new buffer: sole purpose to copy original buffer before parsing so we can print exact command to screen in execvp
char *cmd_token[MAX_ARG];		// array of tokenized commands from parsed buffer	
int args_count;					// argument counter for reference of parsed
} cmd;
void display_prompt() {
printf("sshell$ ");
return;
}
// Reference website code     ALSO NEED TO TRIM BY TAB
void trim_space(char *str) {		
if (str == NULL) return;
char *end;
while (isspace(*str)) str++;				// move pointer at beginning of string past all spaces/tabs
	end = str + strlen(str) - 1;				// end is at the end of the string (even with spaces)
while (end > str && isspace(*end)) end--;	// move end in towards where the actual characters are
    *(end+1) = '\0';							// add null character to denote end of string
return;
}
// In the buffer we are parsing it without parsing by pipes or redirection
void read_command() {
fgets(cmd.orig_buffer, MAX, stdin);			// get user input
int size = strlen(cmd.orig_buffer);
    cmd.orig_buffer[size-1] = '\0'; 				// Take out null line character
strcpy(cmd.new_buffer,cmd.orig_buffer);	
	cmd.new_buffer[size-1] = '\0'; 
trim_space(cmd.orig_buffer);					// remove all whitespace
//size = strlen(cmd.buffer);				// retrieve new size
//printf("New Buffer size is: %d\n",size);
char *args;									// string	
int count = 0;								// counter
	args = strtok(cmd.orig_buffer, " ");				// Return first token
while (args != NULL) {
//printf("args[%d] = %s\n", count, args);
		cmd.cmd_token[count++] = args;			// 	array[i] = args
		args = strtok(NULL, " ");				// store tokens into args: args[0] = ls, args[1] = pwd
	}
	cmd.args_count = count;						// keep track of arguments in struct
return;
}
void run_builtins() {
if (!(strncmp("exit", cmd.cmd_token[0], strlen(cmd.cmd_token[0])))) {
exit(3); // 3 is an arbitrary number for "exit"
	}
}
// Needs to be improved to account for <, >, different placements of &
int valid_command_line() {
if ((!(strncmp("&", cmd.cmd_token[0], 1)))  || 		// strncmp returns a 0 if the strings are equal
	    (!(strncmp(">", cmd.cmd_token[0], 1)))  ||
	    (!(strncmp("<", cmd.cmd_token[0], 1)))) {
return 0;
	}
return 1;
} 
void execute() {
pid_t pid;
int status;
	pid = fork(); 
if (pid == 0) { 									// Child Process
if (!valid_command_line()) { exit(2); }			// Verify we have a valid command - 2 indicates misuse of shell builtins
// Check if the first argument is a builtin command and run it if so
run_builtins();
execvp(cmd.cmd_token[0], cmd.cmd_token);
exit(1);
		} 
else if (pid > 0) {							 	// Parent Process
waitpid(-1, &status, 0);
switch(WEXITSTATUS(status)){
case 2:
fprintf(stderr, "Error: invalid command line\n");
break;
case 3:
fprintf(stderr, "Bye...\n");
exit(0);
default:
fprintf(stderr, "Error: command not found\n");
fprintf(stderr,"+ completed '%s' [%d]\n", cmd.new_buffer, WEXITSTATUS(status)); 
			}
		}
else {
perror("Failed to fork");
exit(1);
	 	}
return;
}
void reset() {
memset(cmd.orig_buffer,0,MAX);
memset(cmd.cmd_token,0,MAX_ARG);
	cmd.args_count = 0;
}
int main(int argc, char *argv[]) {
while (1) {
display_prompt();
read_command();
execute();
reset();
	}
return 0;
}