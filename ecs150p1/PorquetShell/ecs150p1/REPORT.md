# ecs150p1 

## Authors
Samuel Waters
Armand Nasseri


### High-Level Design Choices
We determined that the commands exit, pwd, and cd all needed to take place in the parent
process because these commands were built in and could be handled by the 
parent. We eventually refactored our original implementation to keep a 
consistent amount of abstraction to make our code more efficient.
Our overall data structure relied on an array of structs, where each struct
represented a process to be run in each index of the global array of commands.
This data structure allowed us to pipe each process properly by iterating 
through our command objects in the child and communicating status' to the 
parent.


### Testing
For testing, we ran each command specified in the assignment description
examples to verify we were producing the expected behavior and associated error
messages. Then, we would typically run our own custom commands to account for 
special cases, such as a pipe or redirection character being located at the end,
beginning, or in the middle of a token. When we felt that we had accounted for
all cases, we would then test our sshell on the given sample test script on
the CSIF, identify any issues, and refactor if necessary.


### More Detailed Implementation Decisions:
#### Phase 1:
In the beginning phases of this assignment, we followed the standard 
fork()+exec()+wait() method to lay out the backbone of our shell in main in
order to have our parent spawn off a child process, have that child execute
some command, then have the parent print some "completed" message, or an error.
To determine what system call function for executing all of our commands, we 
decided to use the execvp function. It allowed us to execute the first command
in our data structure of commands as well as whatever successive commands 
followed based on the child processes we created (Ex: ls -la).
#### Phase 2:
For phase 2, we took in user input into a buffer of 512 bytes using fgets(),
then parsed through that user input in read_command() using our 
trim_whitespace() function as well as strtok() in order to produce an array of 
tokens that we could feed to execvp and execute as a command. We stored our 
tokens and number of tokens in a global struct for simplicity, but we recognize 
that it may have been more efficient to pass around a pointer to a struct. 
#### Phase 3:
We basically handled this when implementing our parsing and tokenizing during
phase 2, since we knew that future commands would require more than one token.
#### Phase 4:
We created the function is_builtin(), which returned an integer corresponding
to some builting command (1 for exit, 2 for pwd, 3 for cd, 0 if not a builtin).
In the beginning of our child process, if we found that the command was a
builtin, we'd immediately exit to the parent and run that builtin command with 
the int value returned by is_builti().
#### Phases 5 and 6:
Phase 5 made us modify our parsing in read_command() to account for cases
of redirection characters. We accounted for all valid positions of redirection
(beginning of token, middle, end). Our find_redirection_char() function
returns the position of "<" or ">" in a token as we're parsing through the
entire command line. If there was a redirection character present, we passed
off control to parse_by_redirection(), which handled removing the character
from the associated token (either by use of an additonal strtok() or
incrementing the pointer at the beginning of the token), which then called 
set_file(), which set our infile and outfile variables for later file descriptor
manipulation in dup_handler().
#### Phase 7:
Phase 7 made us refactor our code to account for multiple commands. Instead of
one global struct, we are now using an array of our custom "command" structs
to represetnt one command per entry in our cmds array. Our parse_by_pipes()
function acts similarly to parse_by_redirection(), but instead increments our
index of current command struct, cmds_i, and places the relevant token within
the appropriate command struct. Once we have these separated commands, if we
find that cmds_i is greater than 0 (we have more than one command struct), we're
running execute_with_pipes() in main, which acts similarly to our execute()
function, but additonally iterates through each command struct and manupilates
our file descriptor table for each child process. We create a pipe and our
special file descriptor array fd. On our first command, we're writing to the 
write end of our pipe and reading from STDIN. On every middle command, we're
reading from the read end of the pipe and writing to the write end, and on the
final command, we're once again writing to STDOUT and still reading from the
pipe. We additonally wait for each child to finish individually so that we can
guarantee that one child has finished executing its command before the next
attempts to read that command's result.

### Sources
https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing
-whitespace-in-a-standard-way 
https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/

We would like to cite the trim_space function that we used as a helper
function to trim whitespaces during our tokenization process in
collecting our input.

https://www.geeksforgeeks.org/pipe-system-call/ 
https://linux.die.net/man/2/dup2

We refered to these sources when we were trying to understand the high level
idea of piping our commands properly using the dup2() and pipe() system calls.
