#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

extern char **get_line();
extern char **environ;
char* id;

typedef void (*sighandler_t)(int);
sighandler_t old_sig;

bool back_flag;
char* re_out;
char* re_in;
int pipe_count;

void init_flags(void)
{
	back_flag = 0;
	re_out = NULL;
	re_in = NULL;
	pipe_count = -1;
}

void type_prompt(char* id)
{
   printf("%s# ", id);
}

void kchild(int sig)
{
    if(back_flag == 0)return;
	pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        back_flag--;
    }
    if(back_flag == 0) signal(SIGCHLD, old_sig);
}

void cd(char* dir)
{
	chdir(dir);
}

char** parse(char** args)
{
	if (args[0] == NULL) return NULL;
	if (!strcmp("exit", args[0])) exit(0);
	char** ret = malloc(sizeof(char*)*10);
	int i;
	int j = 0;
	for(i = 0; args[i] != NULL; i++)
	{
		char* s = args[i];
		switch(s[0])
			{
				case '&':
					back_flag++;
					old_sig = signal(SIGCHLD, kchild);
					break;
				case '>':
					if(args[++i] != NULL) re_out = strdup(args[i]);//else error
					else fprintf(stderr, "Output file path not found.\n");
					break;
				case '<':
					if(args[++i] != NULL) re_in = strdup(args[i]);//else error
					else fprintf(stderr, "Input file path not found.\n");
					break;
				case '|':
					pipe_count = i;
					break;
				default:
					ret[j++] = strdup(args[i]);
					break;
			}
	}
	ret[j+1] = NULL;
	return ret;
}

char* strip_path(char* path)
{
	int i;
	char* slash = NULL;
	for(i = 0; path[i] != 0; i++)
	{
		if(path[i] == '/') slash = (path + i);
	}
	if(slash != NULL) return strdup(slash + 1);
}

void try_standard()
{
	if(re_out != NULL) 
	{
		re_out = NULL;
		freopen("/dev/tty", "a", stdout);
	}
	if(re_in != NULL) 
	{
		re_in = NULL;
		freopen("/dev/tty", "r", stdin);
	}
}

void set_standard()
{
	if(re_out != NULL) freopen(re_out, "w", stdout);
	if(re_in != NULL) freopen(re_in, "r", stdin);
}

char* extract_path(char* PATH, int n)
{
    int i;
    for(i = 0; PATH[i] != NULL; i++)
    {
        if(PATH[i] == ':')
        {
            if(!n--) break;
        }
    }
    return strndup(PATH + i, i+1);
}

void execute(char** args)
{
	set_standard();
	if(!strcmp(args[0], "cd"))
	{
		cd(args[1]);
        return;
	}
	int er = execvp(args[0], args);
    char* PATH = getenv("PATH");
    int limit = strlen(PATH);
    int count = 0;
    char* dir = extract_path(PATH, count);
    while(er == -1)
    {
        char* dir = extract_path(PATH, count);
        char* bin = malloc(strlen(dir) + 1 + strlen(args[0]));
		strcpy(bin, dir);
        strcat(bin, "/");
		strcat(bin, args[0]);
		args[0] = bin;
		execvp(args[0], args);
        count++;
        if(count > limit) break;
    }
    perror("execvp");
	exit(1);
}

void execute_pipe(char** args)
{
	char** args1 = malloc(sizeof(char*)*10);
	char** args2 = malloc(sizeof(char*)*10);
	int i;
	for(i = 0; i<pipe_count; i++)
	{
		args1[i] = args[i]; 
	}
	int j;
	for(i = pipe_count, j = 0; args[i]!=NULL; i++, j++)
	{
		args2[i] = args[i];
	}
	int fd[2];// 0 = read, 1 = write
	pipe(fd);
	pid_t pid = fork();
	switch(pid)
	{
		case 0://child
			close(fd[0]);
			dup2(fd[1], fileno(stdout));
			execute(args1);
			break;
		case -1://error
    		printf("fork 1 failed\n");
    		exit(1);
    	default:
    		pid = fork();
    		switch(pid)
    		{
    			case 0://child
    				close(fd[1]);
    				dup2(fd[0], fileno(stdin));
    				execute(args2);
    				break;
    			case -1://error
    				printf("fork 2 failed\n");
    				exit(1);
    			default:
    				break;
    		}
    }
}

int main(int argc, char** argv) 
{	
	int status;
	id = strip_path(argv[0]);
	init_flags();
	int i;
	char **args;

	while(1) 
	{
		type_prompt(id);
		args = get_line();
		if(args == NULL) continue;
    	args = parse(args);
		if(pipe_count >= 0)
		{
			execute_pipe(args);
			continue;
		}
    	pid_t pid = fork();
    	switch(pid)
    	{
    		case 0://child
    			execute(args);
    			exit(1);
    			break;
    		case -1://error
    			printf("fork failed\n");
    			exit(1);
    		default:
    			if(!back_flag) 
    			{
    				wait(&status);
    			}
    	}
  	}
}
