#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX 1024

int parse_command(char* token);
int is_numeric(char* str);
int run_command(char** history, int* his_counter, char** tokens, int* num_tokens);
void pipe_command(char** cmds, int num_cmds);
void read_line(char* lineptr);
int split_line_tok(char* lineptr, char* delim, char** tokens);
void sish_loop();

int main(int argc, char *argv[]) {
  // Enter the shell loop
  sish_loop();

  return 0;
}

// The shell loop
void sish_loop() {
  char* lineptr;  // To store the entire line
  char* templine; // To copy the line into (this is to preserve lineptr after it gets cut up by strtok())
  char* tokens[MAX]; // To store the tokens
  int num_tokens; // To keep track of the number of tokens to make loops easier
  char** history;  // To store the command history
  int his_counter = 0;  // To track where in the command history we want to enter newly entered lines

  // Allocate memory for the history array
  history = (char**)malloc(100 * sizeof(char*));
  
  // The loop
  int run = 1;
  while(run) {
    // Allocate space for the lineptr and the templine
    lineptr = (char*)malloc(MAX * sizeof(char));
    templine = (char*)malloc(MAX * sizeof(char));

    // Prompt the user for input and wait
    printf("sish> ");
    read_line(lineptr);
    strcpy(templine, lineptr);

    // Split line into multiple pipe commands
    char* cmds[MAX];
    int num_cmds = split_line_tok(lineptr, "|", cmds);
    // If the number of commands to be executed less than one, run the command by itself. Else run the pipe_command() function
    if(num_cmds <= 1) {
      num_tokens = split_line_tok(cmds[0], " ", tokens);
      run = run_command(history, &his_counter, tokens, &num_tokens);
    } else {
      pipe_command(cmds, num_cmds);
    }

    // Update the history counter.
    // If the history counter is greater than the maximum allowed commands to be stored,
    // free the first string in the array and shift all of the commands in the list down one array index.
    if(his_counter > 99) {
      his_counter = 99;
      free(history[0]);
      for(size_t i = 0; i < 99; i++) {
        history[i] = history[i + 1];
      }
      free(history[100]);
    }
    // Allocate space for the command to be saved in history
    history[his_counter] = (char*)malloc(MAX * sizeof(char));
    strcpy(history[his_counter], templine);
    his_counter++;

    // Free the space that was allocated for the lineptr
    free(templine);
    free(lineptr);
  }
}

// Read the line entered by the user and store in lineptr
void read_line(char* lineptr) {
  fgets(lineptr, MAX, stdin);
}

// Split the line into tokens. Returns the number of tokens
int split_line_tok(char* lineptr, char* delim, char** tokens) {
  // Store the first token in the token pointer
  char* token = strtok(lineptr, delim);

  // Loop through the string and use strtok() to split the line
  int i = 0;
  while(token != NULL) {
    tokens[i] = token;
    token = strtok(NULL, delim);
    i++;
  }

  // Go to the end of the last token and replace the newline with a null character
  token = tokens[i - 1];
  if(*(token + strlen(token) - 1) == '\n')
    *(token + strlen(token) - 1) = '\0';
  tokens[i - 1] = token;

  // Return the number of tokens
  return i;
}

// Parse the first token to check for a built in command
int parse_command(char* token) {
  if(strcmp(token, "exit") == 0)
    return 0;

  if(strcmp(token, "cd") == 0)
    return 1;

  if(strcmp(token, "history") == 0)
    return 2;

  return -1;
}

// Run a given command
int run_command(char** history, int* his_counter, char** tokens, int* num_tokens) {
  switch(parse_command(tokens[0])) {
      case 0: // Exit
        return 0;
	break;
      case 1: // Change directory
        // Print error if there is not enough or too many arguments
	if(*num_tokens < 2) {
	  printf("Error: Too few arguments for command \'cd\'\n");
	  break;
	}
	if(*num_tokens > 2) {
	  printf("Error: Too many arguments for command \'cd\'\n");
	  break;
	}
	// Call chdir()
	if(chdir(tokens[1]) != 0)
	  printf("Error: cd to %s failed\n", tokens[1]);
	break;
      case 2: // History
        // If history is called by itself print the command history
	if(*num_tokens == 1) {
          for(size_t i = 0; i < *his_counter; i++) {
            printf("%zu ", i);
            printf("%s", history[i]);
          }
	  break;
        }
	// Print error if too many arguments are entered
	if(*num_tokens > 2) {
	  printf("Error: Too many arguments for command \'history\'\n");
	  break;
	}
	// If -c argument is entered clear the history
	if(*num_tokens == 2 && strcmp(tokens[1], "-c") == 0) {
	  *his_counter = 0;
	  break;
	} else if(is_numeric(tokens[1])){ // If a number is entered execute the command at the offset
	  long temp = strtol(tokens[1], NULL, 10);
	  if(temp > *his_counter || temp < 0) {
	    printf("Error: Invalid offset \'%s\' for command \'history\'\n", tokens[1]);
	    break;
	  }
	  // To avoid segmentation fault from command not being put into history yet
	  if(*his_counter == 1 || strcmp(history[0], "history 0\n") == 0) {
	    for(size_t i = 0; i < *his_counter; i++) {
	      printf("%zu ", i);
	      printf("%s", history[i]);
	    }
	    break;
	  }
	  // Allocate resources to run command at given history offset
	  char* temp_toks[MAX];
	  char* temp_line = (char*)malloc(MAX * sizeof(char));
	  strcpy(temp_line, history[temp]);
	  int temp_num = split_line_tok(temp_line, " ", temp_toks);
	  run_command(history, his_counter, temp_toks, &temp_num);
	  free(temp_line);
	  break;
	} else {
	  printf("Error: Invalid argument \'%s\' for command \'history\'\n", tokens[1]);
	  break;
	}
	break;
      default: ; // Attempt to execute command that is entered that is not a built in command
	int child;  // The process id of the child
	int child_status;  // The status of the child that the parent will observe
	char* argv[MAX];  // The arguments passed into the command

	// Get the arguments for the command and NULL terminate the array
	size_t i = 0;
	for(i = 0; i < *num_tokens; i++) {
	  argv[i] = (char*)malloc(MAX * sizeof(char));
	  argv[i] = strcpy(argv[i], tokens[i]);
	}
	argv[i] = NULL;

	// Create the child process
	child = fork();
	if(child == 0) {
	  // Attempt to execute the command
	  int status = execvp(tokens[0], argv);
	  if(status == -1) { // If the command is unrecognized, print an error and exit
	    printf("Unrecognized command: \'%s\'\n", tokens[0]);
	  }
	  exit(0);
	} else { // The parent proces executes this code
	  int tpid;
	  while(tpid != child) {
	    tpid = wait(&child_status); // Wait until the child process terminates
	  }
	}

        break;
    }

  return 1;
}

// The function that is called for a pipe command
void pipe_command(char** cmds, int num_cmds) {
  int prev_pipe, fd[2]; // The prev_pipe that was used by the previous command and the file descriptor

  prev_pipe = STDIN_FILENO; // The prev_pipe starts as STDIN_FILENO to tell the first command to write and not read

  // Create the child process that will execute all of the commands
  int child = fork();
  if(child == 0) { // Code executed by the child process
    for(size_t i = 0; i < num_cmds - 1; i++) { // Loop through all of the commands in the pipe except the last one
      pipe(fd); // Pipe the file descriptor

      char* tokens[MAX]; // The tokens from the current command
      int num_tokens = split_line_tok(cmds[i], " ", tokens); // Get the tokens and the number of tokens
      char* argv[MAX]; // The arguments to be passed into the command

      // Create the argument array
      size_t i = 0;
      for(i = 0; i < num_tokens; i++) {
        argv[i] = (char*)malloc(MAX * sizeof(char));
        argv[i] = strcpy(argv[i], tokens[i]);
      }
      argv[i] = NULL;
    
      // Create a child process to execute a command
      int child = fork();
      if(child == 0) { // Code to be executed by the child process
        if(prev_pipe != STDIN_FILENO) { // Redirect previous pipe to stdin
          dup2(prev_pipe, STDIN_FILENO);
	  close(prev_pipe);
        }

	// Redirect stdout to current pipe
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

	// Attempt to execute the command and print an error and exit if it fails
        execvp(tokens[0], argv);
        printf("Error: Unrecognized command \'%s\'\n", tokens[0]);
        exit(0);
      }

      // Close read and write and save read end of current pipe to use in next command
      close(prev_pipe);
      close(fd[1]);
      prev_pipe = fd[0];

    }
    
    // Get stdin from last pipe
    if(prev_pipe != STDIN_FILENO) {
      dup2(prev_pipe, STDIN_FILENO);
      close(prev_pipe);
    }

    // Get the last command and split it into tokens
    char* tokens[MAX];
    int num_tokens = split_line_tok(cmds[num_cmds - 1], " ", tokens);

    // Attempt to run the command
    run_command(NULL, NULL, tokens, &num_tokens);

    // Close stdin and stdout
    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    // End the child process
    exit(0);
  }
  waitpid(child, NULL, 0); // Parent process waits until all processes terminate
}

// Checks if a string is a number
int is_numeric(char* str)
{
  // Loop through a given string
  while(*str != '\0')
  {
      // Return if any character is not a digit
      if(*str < '0' || *str > '9')
          return 0;
      str++;
  }
  return 1;
}
