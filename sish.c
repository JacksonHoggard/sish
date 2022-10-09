#include <string.h>
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

  while(1) {
    printf("sish> ");
    read_line(lineptr);
    num_tokens = split_line_tok(lineptr, " ", tokens);
    printf("%d\n", num_tokens);

    switch(parse_command(tokens[0])) {
      case 0:
        return;
      default:
        printf("Unrecognized command: \'%s\'\n", tokens[0]);
        break;
    }
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
  *(token + strlen(token) - 1) = '\0';
  tokens[i - 1] = token;

  return i;
}

int parse_command(char* token) {
  char* exit = "exit";

  if(strcmp(token, exit) == 0)
    return 0;

  return -1;
}

