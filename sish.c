#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX 1024

int parse_command(char* token);
void read_line(char* lineptr);
int split_line_tok(char* lineptr, char* delim, char** tokens);
void sish_loop();

int main(int argc, char *argv[]) {
  sish_loop();

  return 0;
}

void sish_loop() {
  char lineptr[MAX];
  char* tokens[MAX];
  char num_tokens;
  char** history;
  int his_counter = 0;

  history = (char**)malloc(100 * sizeof(char*));

  int run = 1;
  while(run) {
    printf("sish> ");
    read_line(lineptr);

    if(his_counter > 99)
      his_counter = 0;
    history[his_counter] = (char*)malloc(MAX * sizeof(char));
    strcpy(history[his_counter], lineptr);
    his_counter++;

    num_tokens = split_line_tok(lineptr, " ", tokens);

    switch(parse_command(tokens[0])) {
      case 0:
        run = 0;
	break;
      case 1:
	if(num_tokens < 2) {
	  printf("Error: Too few arguments for command \'cd\'\n");
	  break;
	}
	if(num_tokens > 2) {
	  printf("Error: Too many arguments for command \'cd\'\n");
	  break;
	}
	if(chdir(tokens[1]) != 0)
	  printf("Error: cd to %s failed\n", tokens[1]);
	break;
      case 2:
	if(num_tokens == 1) {
          for(size_t i = 0; i < his_counter; i++) {
            printf("%zu ", i);
            printf("%s", history[i]);
          }
	  break;
        }
	if(num_tokens > 2) {
	  printf("Error: Too many arguments for command \'history\'\n");
	  break;
	}
	if(num_tokens == 2 && strcmp(tokens[1], "-c") == 0) {
	  printf("clear\n");
	  break;
	} else {
	  printf("Error: Invalid argument \'%s\' for command \'history\'\n", tokens[1]);
	  break;
	}
	break;	
      default:
        printf("Unrecognized command: \'%s\'\n", tokens[0]);
        break;
    }
  }

  free(history);
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
  *(token + strlen(token) - 1) = '\0';
  tokens[i - 1] = token;

  return i;
}

int parse_command(char* token) {
  char* exit = "exit";
  char* cd = "cd";
  char* history = "history";

  if(strcmp(token, exit) == 0)
    return 0;

  if(strcmp(token, cd) == 0)
    return 1;

  if(strcmp(token, history) == 0)
    return 2;

  return -1;
}
