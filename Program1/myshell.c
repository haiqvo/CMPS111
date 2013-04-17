#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

extern char **get_line();

main() {
  int i;
  char **args;
  char * environ[]={NULL};
  char *argv[2];
  char *bin = "/bin/";
  int status;

  while(1) {
    args = get_line();
    if(!strcmp(args[0],"exit")){
      exit(0);
    }
    for(i = 0; args[i] != NULL; i++) {
      printf("Argument %d: %s\n", i, args[i]);
    }
    if(fork() !=0){
       waitpid(-1, &status, 0);
    }else{
        char *exe = malloc(strlen(bin) + strlen(args[0]));
        strcpy(exe, bin);
        strcat(exe, args[0]);
        argv[0] = exe;
        argv[1] = NULL;
        execve(exe, argv, environ);
    }
  }
}

