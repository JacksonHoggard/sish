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
void read_line(char* lineptr);
int split_line_tok(char* lineptr, char* delim, char** tokens);
void sish_loop();

int main(int argc, char *argv[]) {
  sish_loop();

  return 0;
}

void sish_loop() {
  char* lineptr;
  char* templine;
  char* tokens[MAX];
  int num_tokens;
  char** history;
  int his_counter = 0;

  lineptr = (char*)malloc(MAX * sizeof(char));
  templine = (char*)malloc(MAX * sizeof(char));
  history = (char**)malloc(100 * sizeof(char*));

  int run = 1;
  while(run) {
    printf("sish> ");
    read_line(lineptr);
    strcpy(templine, lineptr);

    // Split line into multiple pipe commands
    char* cmds[MAX];
    int num_cmds = split_line_tok(lineptr, "|", cmds);
    for(size_t i = 0; i < num_cmds; i++) {
      num_tokens = split_line_tok(cmds[i], " ", tokens);
      run = run_command(history, &his_counter, tokens, &num_tokens);
    }

    if(his_counter > 99) {
      his_counter = 99;
      free(history[0]);
      for(size_t i = 0; i < 99; i++) {
        history[i] = history[i + 1];
      }
      free(history[100]);
    }
    history[his_counter] = (char*)malloc(MAX * sizeof(char));
    strcpy(history[his_counter], templine);
    his_counter++;
  }
}

void read_line(char* lineptr) {
  fgets(lineptr, MAX, stdin);
}

int split_line_tok(char* lineptr, char* delim, char** tokens) {
  char* token = strtok(lineptr, delim);

  int i = 0;
  while(token != NULL) {
    tokens[i] = token;
    token = strtok(NULL, delim);
    i++;
  }

  token = tokens[i - 1];
  if(*(token + strlen(token) - 1) == '\n')
    *(token + strlen(token) - 1) = '\0';
  tokens[i - 1] = token;

  return i;
}

int parse_command(char* token) {
  if(strcmp(token, "exit") == 0)
    return 0;

  if(strcmp(token, "cd") == 0)
    return 1;

  if(strcmp(token, "history") == 0)
    return 2;

  return -1;
}

int run_command(char** history, int* his_counter, char** tokens, int* num_tokens) {
  switch(parse_command(tokens[0])) {
      case 0:
        return 0;
	break;
      case 1:
	if(*num_tokens < 2) {
	  printf("Error: Too few arguments for command \'cd\'\n");
	  break;
	}
	if(*num_tokens > 2) {
	  printf("Error: Too many arguments for command \'cd\'\n");
	  break;
	}
	if(chdir(tokens[1]) != 0)
	  printf("Error: cd to %s failed\n", tokens[1]);
	break;
      case 2:
	if(*num_tokens == 1) {
          for(size_t i = 0; i < *his_counter; i++) {
            printf("%zu ", i);
            printf("%s", history[i]);
          }
	  break;
        }
	if(*num_tokens > 2) {
	  printf("Error: Too many arguments for command \'history\'\n");
	  break;
	}
	if(*num_tokens == 2 && strcmp(tokens[1], "-c") == 0) {
	  *his_counter = 0;
	  break;
	} else if(is_numeric(tokens[1])){
	  long temp = strtol(tokens[1], NULL, 10);
	  if(temp > *his_counter || temp < 0) {
	    printf("Error: Invalid offset \'%s\' for command \'history\'\n", tokens[1]);
	    break;
	  }
	  if(*his_counter == 1 || strcmp(history[0], "history 0\n") == 0) {
	    for(size_t i = 0; i < *his_counter; i++) {
	      printf("%zu ", i);
	      printf("%s", history[i]);
	    }
	    break;
	  }
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
      default: ;
	int child;
	int child_status;
	char* argv[MAX];

	size_t i = 0;
	for(i = 0; i < *num_tokens; i++) {
	  argv[i] = (char*)malloc(MAX * sizeof(char));
	  argv[i] = strcpy(argv[i], tokens[i]);
	}
	argv[i] = NULL;

	child = fork();
	if(child == 0) {
	  int status = execvp(tokens[0], argv);
	  if(status == -1) {
	    printf("Unrecognized command: \'%s\'\n", tokens[0]);
	  }
	  exit(0);
	} else {
	  int tpid;
	  while(tpid != child) {
	    tpid = wait(&child_status);
	  }
	}

        break;
    }

  return 1;
}

int is_numeric(char* str)
{
  while(*str != '\0')
  {
      if(*str < '0' || *str > '9')
          return 0;
      str++;
  }
  return 1;
}
