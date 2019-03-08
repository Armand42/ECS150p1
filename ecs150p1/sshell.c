
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#define MAX 512
#define MAX_ARG 16
#define IN 0
#define OUT 1

struct command {
	char *cmd_token[MAX_ARG];		// array of tokenized commands from parsed buffer	
	int args_count;					// argument counter for reference of parsed
	char *infile;
	char *outfile;
} cmds[MAX_ARG];

char orig_buffer[MAX];
char new_buffer[MAX];
int cmds_i=0;

// void print_args() {
// 	for (int i=0; i<cmds[cmds_i].args_count; i++) {
// 		printf("arg%d: %s\n", i, cmds[cmds_i].cmd_token[i]);
// 	}
// }

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

void dup_handler() {
	int fd;	
	
	if (cmds[cmds_i].infile != NULL) {			// <
		fd = open(cmds[cmds_i].infile, O_RDONLY, 644);
		// check error on open
		if (fd == -1) {
			//printf("Error: cannot open input file\n");
			// Not able to open input file / file doesn't exist
			exit(3);
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
    }
    if (cmds[cmds_i].outfile != NULL) {		// >
		fd = open(cmds[cmds_i].outfile, O_WRONLY | O_CREAT, 0644);
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}

	/*else if (cmds[cmds_i].infile == NULL) {
		fprintf(stderr, "Error: no input file\n");
	}
	else if (cmds[cmds_i].outfile == NULL) {
		fprintf(stderr, "Error: no output file\n");
	}*/
	return;
}

int find_redirection_char(char str[]) {
	for (int i=0; i<strlen(str); i++) {
		if ((!strncmp(&str[i], "<", 1)) ||
		   (!strncmp(&str[i], ">", 1)))	{
			return i;
		}
	}
	return -1;
}

int find_pipe_char(char str[]) {
	for (int i=0; i<strlen(str); i++) {
		if ((!strncmp(&str[i], "|", 1))) {
			return i;
		}
	}
	return -1;
}


// Sets either the infile or outfile for use with dup_handler. arg is the file name and redir_char is the redirection character
void set_file(char *filename, char redir_char) {
	size_t fileLen = strlen(filename);
	// set infile
	if (redir_char == '<') {
		cmds[cmds_i].infile = malloc(fileLen*(sizeof(char)+1));
		for (int i=0; i<fileLen; i++) {
			// printf("char %d: %c\n", i, filename[i]);
			cmds[cmds_i].infile[i] = filename[i];
		}
	}
	// set outfile
	else {
		cmds[cmds_i].outfile = malloc(fileLen*(sizeof(char)+1));
		for (int i=0; i<fileLen; i++) {
			// printf("char %d: %c\n", i, filename[i]);
			cmds[cmds_i].outfile[i] = filename[i];
		}
	}
	return;
}

int parse_by_pipes(char *arg, int pos) {
	// Either at the beginning of a token or alone as the whole token
	if (pos == 0) {
		// The whole argument is the pipe char
		if (strlen(arg)==1) {
			// Onto the next command sequence
			cmds_i++;
		}
		else { // At the beginning of a token (like |grep)
			// Onto next command sequence
			cmds_i++;
			cmds[cmds_i].cmd_token[cmds[cmds_i].args_count++] = arg+1;
		}
	}
	// pipe char either at the end of the arg or somewhere in the middle
	else {
		// At the end of the argument (last element in arg to command, ex: echo toto| grep to) or in the middle somewhere
		// add argument (without redir char) to args array
		strncpy(cmds[cmds_i].cmd_token[cmds[cmds_i].args_count++], arg, pos);
		cmds_i++;
		if (pos < strlen(arg)-1) {
			arg = arg+(pos+1);
			cmds[cmds_i].cmd_token[cmds[cmds_i].args_count++] = arg;
		}
	}	
	return 0;
}

int parse_by_redirection(char *arg, int pos) {
	char redir_char = arg[pos];
	char redir_char_as_string[1] = {redir_char};
	// Either at the beginning of a token or alone as the whole token
	if (pos == 0) {
		// The whole argument is the redirection char
		if (strlen(arg)==1) {
			// do another strtok on whitespace to find in/outfile
			arg = strtok(NULL, " ");
			// printf("%s\n", arg);
			if (arg == NULL) {

				if (redir_char == '<') {
				// no file
				fprintf(stderr, "Error: no input file\n");
				}
				else {
					fprintf(stderr, "Error: no output file\n");
				}
				return -1;
			}
			else { // there's actually an argument there -- we got a file to set as in/out file
				// and set infile/outfile to be new arg
				set_file(arg, redir_char);
			}
		}
		else { // At the beginning of a token (like <hello.txt)
			// do a strtok on the redirection character
			set_file(strtok(arg, redir_char_as_string), redir_char);
		}
	}
	// redirection char either at the end of the arg or somewhere in the middle
	else if (pos > 0) {
		char *newArg;
		// At the end of the argument (last element in arg to command, ex: grep toto< file.txt)
		if (pos == strlen(arg)-1) {
			set_file(strtok(NULL, redir_char_as_string), redir_char);
			// remove redir char from end of token
			newArg = strtok(arg, redir_char_as_string);
			// add argument (without redir char) to args array
			cmds[cmds_i].cmd_token[cmds[cmds_i].args_count++] = newArg;
		}
		// In the middle somewhere
		else {
			// get argument before redir char
			newArg = strtok(arg, redir_char_as_string);
			// put that in our args array
			cmds[cmds_i].cmd_token[cmds[cmds_i].args_count++] = newArg;
			// get the filename from our full arg and set in/outfile
			set_file(arg+(pos+1), redir_char);
			arg = strtok(NULL, redir_char_as_string);
		}
	} 
	// no redirection character in the arg -- just add the current arg to our array and increment args count
	else {
		cmds[cmds_i].cmd_token[cmds[cmds_i].args_count++] = arg;
	}
	return 0;
}

// In the buffer we are parsing it without parsing by pipes or redirection
int read_command() {
	char *args; 
	memset(orig_buffer, 0, MAX);									// string for individual arguments
	fgets(orig_buffer, MAX, stdin);			// get user input
	// printf("orig buff: %s\n", orig_buffer);
	// if (orig_buffer[0] == '\n') {
	// 	return -1;
	// }
	// tester code
	/*
     * Echoes command line to stdout if fgets read from a file and not
     * the terminal (which is the case with the test script)
     */
    if (!isatty(STDIN_FILENO)) {
        printf("%s", orig_buffer);
        fflush(stdout);
    }

	int size = strlen(orig_buffer);
    orig_buffer[size-1] = '\0'; 				// Take out null line character
	strcpy(new_buffer,orig_buffer);	
	new_buffer[size-1] = '\0'; 
    trim_space(orig_buffer);					// remove all whitespace
	// printf("size of origbuffer[0]: %lu\n", sizeof(orig_buffer[0]));
	// if (orig_buffer[0] == '\n' || orig_buffer[0] == 32) {
	// 	return -1;
	// }
	
	// int count = 0;									// counter
	args = strtok(orig_buffer, " ");				// Return first token

	while (args != NULL) {
		int pipe_pos = find_pipe_char(args);

		int redir_pos = find_redirection_char(args);
		if (parse_by_redirection(args, redir_pos) == -1) {
			return -1;
		}
		if (pipe_pos >= 0) {
			// printf("pipe pos: %d\n", pipe_pos);			
			parse_by_pipes(args, pipe_pos);
		}
		args = strtok(NULL, " ");						// store tokens into args: args[0] = ls, args[1] = pwd
		// cmd.args_count++;
	}
	return 0;
}

/*
 * 1 = exit, 2 = pwd, 3 = cd
 */
int is_builtin() {
	// printf("first tok: %s\n", (cmds[cmds_i].cmd_token[0]));
	if (!cmds[cmds_i].cmd_token[0]) {
		return -1;
	}
	size_t len = strlen(cmds[cmds_i].cmd_token[0]);
	if (!strncmp("exit", cmds[cmds_i].cmd_token[0], len) && len==4) {
		return 1;
	}
	if (!strncmp("pwd", cmds[cmds_i].cmd_token[0], len) && len==3) {
		return 2;
	}
	if (!strncmp("cd", cmds[cmds_i].cmd_token[0], len) && len==2) {
		return 3;
	}
	return 0;
}

void run_builtins(int builtin_num, int *status) {
	char buf[MAX];
	switch(builtin_num){
	// exit
	case 1:
		fprintf(stderr, "Bye...\n");
		exit(0);
	// pwd
	case 2:
		getcwd(buf, MAX);
		printf("%s\n", buf);
		break;
	// cd
	case 3:
		// Check if chdir succeeds or fails
		if (chdir(cmds[cmds_i].cmd_token[1]) != 0) {
			fprintf(stderr, "Error: no such directory\n");
			*status = 1;
		}
	}	
}

// Needs to be improved to account for <, >, different placements of &
int valid_command_line() {
	if ((!(strncmp("&", cmds[cmds_i].cmd_token[0], 1)))  || 		// strncmp returns a 0 if the strings are equal
	    (!(strncmp(">", cmds[cmds_i].cmd_token[0], 1)))  ||
	    (!(strncmp("<", cmds[cmds_i].cmd_token[0], 1)))) {
		return 0;
	}
	return 1;
} 

void execute_with_pipes() {
	pid_t pid[cmds_i];			// you know how many commands
	int fd[2];
	int savedout;
	int status[cmds_i+1];		  // array of status'
	int builtin = is_builtin();
	// 3 cases if in beginnig duplicate the output only
	// in middle of pipes dup
	//assign each cmd to fork
	// cmds_i++;
	//printf("num cmd: %d\n", cmds_i);

	

	for (int i = 0; i <= cmds_i; i++){
		pipe(fd);
		pid[i] = fork(); // assigning current process to be forked

		if (pid[i] == 0) { 									// Child Process
			
			if (!valid_command_line()) { // if not a valid command line
				exit(2); 
			}			
			
			if (builtin) {
				exit(0);
			} 

			if (strlen(cmds[i].infile)>0 || strlen(cmds[i].outfile)>0) {
				//dup_handler();
			}
			if (i == 0) {							// first case
				if (strlen(cmds[i].outfile)>0) {				// if redirection found in first cmd
					exit(4);
				}
				else {
				//printf("first!\n");
					dup2(fd[1], STDOUT_FILENO);			// write stdout to file descriptor 
				}
			}
			else if (i < cmds_i) {
				//printf("second!\n");
				dup2(savedout,STDIN_FILENO);		// middle, read savedout to stdin
				dup2(fd[1],STDOUT_FILENO);			// write stdout to file descriptor 
			}
			else if (i == cmds_i) {					// only sending to STDOUT (last case)
				//printf("last!\n");
				if (strlen(cmds[i].infile)>0) {				// if redirection found in last cmd
					exit(5);
				}
				else {
				// if outfile present 
				// else dup
					dup2(savedout,STDIN_FILENO);		// transfer content from last times output as input
				}
			}
			
			status[i] = execvp(cmds[i].cmd_token[0], cmds[i].cmd_token);
			exit(1);

			
		} 	// end of child
		else if (pid[i] == -1) {
			perror("Failed to fork");
			exit(1);
		} 
		else { //parent
			close(fd[1]);			// closes to prevent hanging, 
			savedout = fd[0];		//transfer content to file descriptor
			waitpid(pid[i], &status[i], 0);

			switch(WEXITSTATUS(status[i])){
			case 5:
				fprintf(stderr, "Error: mislocated input redirection\n");
				return;
			case 4:
				fprintf(stderr, "Error: mislocated output redirection\n");
				return;
			case 3:
				fprintf(stderr, "Error: cannot open input file\n");
				return;

			case 2:
				fprintf(stderr, "Error: invalid command line\n");
				return;
			case 1:
				fprintf(stderr, "Error: command not found\n");
				fprintf(stderr,"+ completed '%s' [%d]\n", new_buffer, WEXITSTATUS(status[i]));
				break;
			case 0:
				run_builtins(builtin, &status[i]);
			default:
				if (!builtin) {
					status[i] = WEXITSTATUS(status[i]);
				}
				//fprintf(stderr,"+ completed '%s' [%d]\n", new_buffer, status[i]); 
			}
		}
		
	} // end loop
	fprintf(stderr,"+ completed '%s' ", new_buffer);
	for (int i = 0; i <= cmds_i; i++) {
		printf("[%d]", status[i]);
	}
	printf("\n");
	//  echo hello world | grep hello | wc -l
	return;
}

void execute() {
	pid_t pid;
	int status;
	int builtin = is_builtin();
	pid = fork(); 

	if (pid == 0) { 									// Child Process
		// print_args();
		// printf("infile: %s\n", cmds[cmds_i].infile);
		// printf("outfile: %s\n", cmds[cmds_i].outfile);
		if (!valid_command_line()) { exit(2); }			// Verify we have a valid command - 2 indicates misuse of shell builtins
		// run builtins if they exist
		if (builtin) {
			exit(0);
		} // otherwise we run other commands
		if (cmds[cmds_i].infile || cmds[cmds_i].outfile) {
			dup_handler();
		}
		execvp(cmds[cmds_i].cmd_token[0], cmds[cmds_i].cmd_token);
		exit(1);
	} 

	else if (pid > 0) {							 	// Parent Process
		waitpid(-1, &status, 0);
		switch(WEXITSTATUS(status)){
		case 3:
			fprintf(stderr, "Error: cannot open input file\n");
			break;

		case 2:
			fprintf(stderr, "Error: invalid command line\n");
			break;
		case 1:
			fprintf(stderr, "Error: command not found\n");
			fprintf(stderr,"+ completed '%s' [%d]\n", new_buffer, WEXITSTATUS(status));
			break;
		case 0:
			run_builtins(builtin, &status);
		default:
			if (!builtin) {
				status = WEXITSTATUS(status);
			}
			fprintf(stderr,"+ completed '%s' [%d]\n", new_buffer, status); 
		}
	}
	else {
		perror("Failed to fork");
		exit(1);
	}
	return;
}

void reset() {
	// printf("%lu\n", sizeof(cmds[cmds_i].cmd_token[0]));
	// Remember to reset the rest of our struct
	memset(orig_buffer,0, MAX);
	strcpy(orig_buffer, "\0");
	memset(new_buffer,0, MAX);
	memset(cmds[cmds_i].cmd_token,0,MAX_ARG*8);
	free(cmds[cmds_i].infile);
	free(cmds[cmds_i].outfile);
	cmds[cmds_i].args_count = 0;
	cmds_i = 0;
	// for (int i=0; i<=cmds_i; i++) {
	// 	for (int j=0; j<cmds[i].args_count; j++) {
	// 		free(cmds[i].cmd_token[j]);
	// 	}
	// }
}


int main(int argc, char *argv[]) {
	while (1) {
		display_prompt();
		if (read_command() != -1) {
			if (cmds_i >= 1){
				execute_with_pipes();
			}
			else {
				execute();
			}
		}
		reset();
	}
	
 	return 0;
}
